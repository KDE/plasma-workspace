/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#include "appmenu.h"
#include "../c_ptr.h"
#include "appmenu_dbus.h"
#include "appmenu_debug.h"
#include "appmenuadaptor.h"
#include "kdbusimporter.h"
#include "menuimporteradaptor.h"
#include "verticalmenu.h"

#include <QApplication>
#include <QDBusInterface>
#include <QMenu>
#include <QPainter>
#include <QRasterWindow>
#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandwindow_p.h>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <kpluginfactory.h>

K_PLUGIN_FACTORY_WITH_JSON(AppMenuFactory, "appmenu.json", registerPlugin<AppMenuModule>();)

class ToplevelWindow : public QRasterWindow
{
    void paintEvent(QPaintEvent *) override
    {
        QPainter(this).fillRect(geometry(), Qt::transparent);
    }
};

AppMenuModule::AppMenuModule(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_appmenuDBus(new AppmenuDBus(this))
{
    reconfigure();

    m_appmenuDBus->connectToBus();

    connect(m_appmenuDBus, &AppmenuDBus::appShowMenu, this, &AppMenuModule::slotShowMenu);
    connect(m_appmenuDBus, &AppmenuDBus::reconfigured, this, &AppMenuModule::reconfigure);

    // transfer our signals to dbus
    connect(this, &AppMenuModule::showRequest, m_appmenuDBus, &AppmenuDBus::showRequest);
    connect(this, &AppMenuModule::menuHidden, m_appmenuDBus, &AppmenuDBus::menuHidden);
    connect(this, &AppMenuModule::menuShown, m_appmenuDBus, &AppmenuDBus::menuShown);

    m_menuViewWatcher = new QDBusServiceWatcher(QStringLiteral("org.kde.kappmenuview"),
                                                QDBusConnection::sessionBus(),
                                                QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                                this);

    auto setupMenuImporter = [this]() {
        QDBusConnection::sessionBus().connect({},
                                              {},
                                              QStringLiteral("com.canonical.dbusmenu"),
                                              QStringLiteral("ItemActivationRequested"),
                                              this,
                                              SLOT(itemActivationRequested(int, uint)));

        // Setup a menu importer if needed
        if (!m_menuImporter) {
            m_menuImporter = new MenuImporter(this);
            connect(m_menuImporter, &MenuImporter::WindowRegistered, this, &AppMenuModule::slotWindowRegistered);
            m_menuImporter->connectToBus();
        }
    };
    connect(m_menuViewWatcher, &QDBusServiceWatcher::serviceRegistered, this, setupMenuImporter);
    connect(m_menuViewWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
        Q_UNUSED(service)
        QDBusConnection::sessionBus().disconnect({},
                                                 {},
                                                 QStringLiteral("com.canonical.dbusmenu"),
                                                 QStringLiteral("ItemActivationRequested"),
                                                 this,
                                                 SLOT(itemActivationRequested(int, uint)));
        delete std::exchange(m_menuImporter, nullptr);
    });

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.kappmenuview"))) {
        setupMenuImporter();
    }

#if HAVE_X11
    if (auto interface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>(); !interface || !interface->connection()) {
        m_xcbConn = xcb_connect(nullptr, nullptr);
    }
#endif
    if (qGuiApp->platformName() == QLatin1String("wayland")) {
        auto connection = KWayland::Client::ConnectionThread::fromApplication();
        KWayland::Client::Registry registry;
        registry.create(connection);
        connect(&registry, &KWayland::Client::Registry::plasmaShellAnnounced, this, [this, &registry](quint32 name, quint32 version) {
            m_plasmashell = registry.createPlasmaShell(name, version, this);
        });
        registry.setup();
        connection->roundtrip();
    }
}

AppMenuModule::~AppMenuModule()
{
#if HAVE_X11
    if (m_xcbConn) {
        xcb_disconnect(m_xcbConn);
    }
#endif
}

void AppMenuModule::slotWindowRegistered(WId id, const QString &serviceName, const QDBusObjectPath &menuObjectPath)
{
#if HAVE_X11
    auto interface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    auto *c = interface ? interface->connection() : nullptr;
    if (!c) {
        c = m_xcbConn;
    }

    if (c) {
        static xcb_atom_t s_serviceNameAtom = XCB_ATOM_NONE;
        static xcb_atom_t s_objectPathAtom = XCB_ATOM_NONE;

        auto setWindowProperty = [c](WId id, xcb_atom_t &atom, QByteArrayView name, QByteArrayView value) {
            if (atom == XCB_ATOM_NONE) {
                const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(c, false, name.length(), name.constData());
                UniqueCPointer<xcb_intern_atom_reply_t> reply{xcb_intern_atom_reply(c, cookie, nullptr)};
                if (!reply) {
                    return;
                }
                atom = reply->atom;
                if (atom == XCB_ATOM_NONE) {
                    return;
                }
            }

            auto cookie = xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, id, atom, XCB_ATOM_STRING, 8, value.length(), value.constData());
            if (xcb_generic_error_t *error = xcb_request_check(c, cookie)) {
                qCWarning(APPMENU_DEBUG) << "Error changing property" << name << "on window" << id << "error code:" << error->error_code;
                free(error);
                return;
            }
        };

        // TODO only set the property if it doesn't already exist

        constexpr QByteArrayView s_x11AppMenuServiceNamePropertyName("_KDE_NET_WM_APPMENU_SERVICE_NAME");
        constexpr QByteArrayView s_x11AppMenuObjectPathPropertyName("_KDE_NET_WM_APPMENU_OBJECT_PATH");

        setWindowProperty(id, s_serviceNameAtom, s_x11AppMenuServiceNamePropertyName, serviceName.toUtf8());
        setWindowProperty(id, s_objectPathAtom, s_x11AppMenuObjectPathPropertyName, menuObjectPath.path().toUtf8());
    }
#endif
}

void AppMenuModule::slotShowMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId)
{
    if (!m_menuImporter) {
        return;
    }

    // If menu visible, hide it
    if (m_menu && m_menu.data()->isVisible()) {
        m_menu.data()->hide();
        return;
    }

    // dbus call by user (for khotkey shortcut)
    if (x == -1 || y == -1) {
        // We do not know kwin button position, so tell kwin to show menu
        Q_EMIT showRequest(serviceName, menuObjectPath, actionId);
        return;
    }

    auto *importer = new KDBusMenuImporter(serviceName, menuObjectPath.path(), this);
    QMetaObject::invokeMethod(importer, "updateMenu", Qt::QueuedConnection);
    disconnect(importer, nullptr, this, nullptr); // ensure we don't popup multiple times in case the menu updates again later

    connect(importer, &KDBusMenuImporter::menuUpdated, this, [=, this](QMenu *m) {
        QMenu *menu = importer->menu();
        if (!menu || menu != m) {
            return;
        }
        m_menu = qobject_cast<VerticalMenu *>(menu);

        m_menu.data()->setServiceName(serviceName);
        m_menu.data()->setMenuObjectPath(menuObjectPath);

        connect(m_menu.data(), &QMenu::aboutToHide, this, [this, importer] {
            hideMenu();
            importer->deleteLater();
        });

        if (m_plasmashell) {
            QScreen *screen = QGuiApplication::screenAt(QPoint(x, y));
            if (!screen) {
                screen = QGuiApplication::primaryScreen();
            }

            const QRect screenRect = screen->geometry();
            if (!m_menu->isVisible()) {
                // We create a invisible toplevel so the menu can be an xdg_popup which is important
                // to have the expected UX of an menu. By using the ToolTip role it cannot receive
                // focus which is important because some apps misbehave when they dont have focus when
                // a menu is triggered
                auto toplevelWindow = new ToplevelWindow;
                toplevelWindow->setFlag(Qt::FramelessWindowHint);
                toplevelWindow->QObject::setParent(menu);
                toplevelWindow->setGeometry(QRect(screenRect.topLeft(), QSize(1, 1)));
                auto surface = KWayland::Client::Surface::fromWindow(toplevelWindow);
                auto plasmaSurface = m_plasmashell->createSurface(surface, surface);
                plasmaSurface->setSkipSwitcher(true);
                plasmaSurface->setSkipTaskbar(true);
                plasmaSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::ToolTip);
                plasmaSurface->setPosition({x - 1, y - 1});
                toplevelWindow->show();
                connect(m_menu, &QMenu::aboutToShow, toplevelWindow, [toplevelWindow, this] {
                    m_menu->windowHandle()->setTransientParent(toplevelWindow);
                });
                ensureSerial(toplevelWindow);
            }
            m_menu.data()->popup(screenRect.topLeft());
        } else {
            m_menu.data()->popup(QPoint(x, y) / qApp->devicePixelRatio());
        }

        QAction *actiontoActivate = importer->actionForId(actionId);

        Q_EMIT menuShown(serviceName, menuObjectPath);

        if (actiontoActivate) {
            m_menu.data()->setActiveAction(actiontoActivate);
        }
    });
}

void AppMenuModule::hideMenu()
{
    if (m_menu) {
        Q_EMIT menuHidden(m_menu.data()->serviceName(), m_menu->menuObjectPath());
    }
}

void AppMenuModule::itemActivationRequested(int actionId, uint timeStamp)
{
    Q_UNUSED(timeStamp);
    Q_EMIT showRequest(message().service(), QDBusObjectPath(message().path()), actionId);
}

// this method is not really used anymore but has to be kept for DBus compatibility
void AppMenuModule::reconfigure()
{
}

void AppMenuModule::ensureSerial(QWindow *w)
{
    // HACK we need an input serial to create popups but Qt only sets them on click
    if (auto waylandApp = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
        if (!waylandApp->lastInputSerial()) {
            auto waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow *>(w->handle());
            if (waylandWindow) {
                const auto device = waylandWindow->display()->currentInputDevice();
                waylandWindow->display()->setLastInputDevice(device, 1, waylandWindow);
            }
        }
    }
}

#include "appmenu.moc"

#include "moc_appmenu.cpp"
