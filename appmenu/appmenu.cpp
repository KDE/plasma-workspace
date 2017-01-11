/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>
  Copyright (c) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include <config-X11.h>

#include "appmenu.h"
#include "kdbusimporter.h"
#include "menuimporteradaptor.h"
#include "appmenuadaptor.h"
#include "appmenu_dbus.h"
#include "verticalmenu.h"

#include <QDBusInterface>
#include <QMenu>

#include <KConfig>
#include <KConfigGroup>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <KSharedConfig>
#include <KWindowSystem>

#if HAVE_X11
#include <QX11Info>
#include <xcb/xcb.h>
#endif

static const QByteArray s_x11AppMenuServiceNamePropertyName = QByteArrayLiteral("_KDE_NET_WM_APPMENU_SERVICE_NAME");
static const QByteArray s_x11AppMenuObjectPathPropertyName = QByteArrayLiteral("_KDE_NET_WM_APPMENU_OBJECT_PATH");

K_PLUGIN_FACTORY_WITH_JSON(AppMenuFactory,
                           "appmenu.json",
                           registerPlugin<AppMenuModule>();)

AppMenuModule::AppMenuModule(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent),
    m_appmenuDBus(new AppmenuDBus(this))
{
    reconfigure();

    m_appmenuDBus->connectToBus();

    connect(m_appmenuDBus, &AppmenuDBus::appShowMenu, this, &AppMenuModule::slotShowMenu);
    connect(m_appmenuDBus, &AppmenuDBus::reconfigured, this, &AppMenuModule::reconfigure);

    // transfer our signals to dbus
    connect(this, &AppMenuModule::showRequest, m_appmenuDBus, &AppmenuDBus::showRequest);
    connect(this, &AppMenuModule::menuHidden, m_appmenuDBus, &AppmenuDBus::menuHidden);
    connect(this, &AppMenuModule::menuShown, m_appmenuDBus, &AppmenuDBus::menuShown);

    QDBusConnection::sessionBus().connect({}, {}, QStringLiteral("com.canonical.dbusmenu"),
                                                  QStringLiteral("ItemActivationRequested"),
                                          this, SLOT(itemActivationRequested(int,uint)));
}

AppMenuModule::~AppMenuModule() = default;

void AppMenuModule::slotWindowRegistered(WId id, const QString &serviceName, const QDBusObjectPath &menuObjectPath)
{
#ifdef HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        auto *c = QX11Info::connection();

        static xcb_atom_t s_serviceNameAtom = XCB_ATOM_NONE;
        static xcb_atom_t s_objectPathAtom = XCB_ATOM_NONE;

        auto setWindowProperty = [c](WId id, xcb_atom_t &atom, const QByteArray &name, const QByteArray &value) {
            if (atom == XCB_ATOM_NONE) {
                const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(c, false, name.length(), name.constData());
                QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> reply(xcb_intern_atom_reply(c, cookie, Q_NULLPTR));
                if (reply.isNull()) {
                    return;
                }
                atom = reply->atom;
                if (atom == XCB_ATOM_NONE) {
                    return;
                }
            }

            xcb_change_property(c, XCB_PROP_MODE_REPLACE, id, atom, XCB_ATOM_STRING,
                                    8, value.length(), value.constData());
        };

        // TODO only set the property if it doesn't already exist

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

    //dbus call by user (for khotkey shortcut)
    if (x == -1 || y == -1) {
        // We do not know kwin button position, so tell kwin to show menu
        emit showRequest(serviceName, menuObjectPath, actionId);
        return;
    }

    auto *importer = new KDBusMenuImporter(serviceName, menuObjectPath.path(), this);
    QMetaObject::invokeMethod(importer, "updateMenu", Qt::QueuedConnection);
    disconnect(importer, 0, this, 0); // ensure we don't popup multiple times in case the menu updates again later

    connect(importer, &KDBusMenuImporter::menuUpdated, this, [=] {
        QMenu *menu = importer->menu();
        if (!menu) {
            return;
        }
        m_menu = qobject_cast<VerticalMenu*>(menu);

        m_menu.data()->setServiceName(serviceName);
        m_menu.data()->setMenuObjectPath(menuObjectPath);

        connect(m_menu.data(), &QMenu::aboutToHide, this, [this, importer] {
            hideMenu();
            importer->deleteLater();
        });

        //m_menuImporter->fakeUnityAboutToShow(serviceName, menuObjectPath);

        m_menu.data()->popup(QPoint(x, y));

        emit menuShown(serviceName, menuObjectPath);

        if (m_waitingAction) {
            m_menu.data()->setActiveAction(m_waitingAction);
            m_waitingAction = nullptr;
        }
    });
}

void AppMenuModule::hideMenu()
{
    if (m_menu) {
        emit menuHidden(m_menu.data()->serviceName(), m_menu->menuObjectPath());
    }
}

void AppMenuModule::itemActivationRequested(int winId, uint action)
{
    Q_UNUSED(winId);
    emit showRequest(message().service(), QDBusObjectPath(message().path()), action);
}

// reload settings
void AppMenuModule::reconfigure()
{
    m_waitingAction = nullptr;

    hideMenu(); // hide window decoration menu if exists

    KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("Appmenu Style"));
    const QString &menuStyle = config.readEntry("Style", "InApplication");
    // TODO enum or Kconfigxt or what not?
    if (menuStyle == QLatin1String("InApplication")) {
        delete m_menuImporter;
        m_menuImporter = nullptr;
        return;
    }

    // Setup a menu importer if needed
    if (!m_menuImporter) {
        m_menuImporter = new MenuImporter(this);
        connect(m_menuImporter, &MenuImporter::WindowRegistered, this, &AppMenuModule::slotWindowRegistered);
        m_menuImporter->connectToBus();
    }
}

#include "appmenu.moc"
