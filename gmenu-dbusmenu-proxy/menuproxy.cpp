/*
 * Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "menuproxy.h"

#include <config-X11.h>

#include "debug.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>

#include <KConfigGroup>
#include <KDirWatch>
#include <KSharedConfig>
#include <KWindowSystem>

#include <QX11Info>
#include <xcb/xcb.h>

#include "window.h"

static const QString s_ourServiceName = QStringLiteral("org.kde.plasma.gmenu_dbusmenu_proxy");

static const QString s_dbusMenuRegistrar = QStringLiteral("com.canonical.AppMenu.Registrar");

static const QByteArray s_gtkUniqueBusName = QByteArrayLiteral("_GTK_UNIQUE_BUS_NAME");

static const QByteArray s_gtkApplicationObjectPath = QByteArrayLiteral("_GTK_APPLICATION_OBJECT_PATH");
static const QByteArray s_unityObjectPath = QByteArrayLiteral("_UNITY_OBJECT_PATH");
static const QByteArray s_gtkWindowObjectPath = QByteArrayLiteral("_GTK_WINDOW_OBJECT_PATH");
static const QByteArray s_gtkMenuBarObjectPath = QByteArrayLiteral("_GTK_MENUBAR_OBJECT_PATH");
// that's the generic app menu with Help and Options and will be used if window doesn't have a fully-blown menu bar
static const QByteArray s_gtkAppMenuObjectPath = QByteArrayLiteral("_GTK_APP_MENU_OBJECT_PATH");

static const QByteArray s_kdeNetWmAppMenuServiceName = QByteArrayLiteral("_KDE_NET_WM_APPMENU_SERVICE_NAME");
static const QByteArray s_kdeNetWmAppMenuObjectPath = QByteArrayLiteral("_KDE_NET_WM_APPMENU_OBJECT_PATH");

static const QString s_gtkModules = QStringLiteral("gtk-modules");
static const QString s_appMenuGtkModule = QStringLiteral("appmenu-gtk-module");

MenuProxy::MenuProxy()
    : QObject()
    , m_xConnection(QX11Info::connection())
    , m_serviceWatcher(new QDBusServiceWatcher(this))
    , m_gtk2RcWatch(new KDirWatch(this))
    , m_writeGtk2SettingsTimer(new QTimer(this))
{
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration);
    m_serviceWatcher->addWatchedService(s_dbusMenuRegistrar);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &service) {
        Q_UNUSED(service);
        qCDebug(DBUSMENUPROXY) << "Global menu service became available, starting";
        init();
    });
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
        Q_UNUSED(service);
        qCDebug(DBUSMENUPROXY) << "Global menu service disappeared, cleaning up";
        teardown();
    });

    // It's fine to do a blocking call here as we're a separate binary with no UI
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(s_dbusMenuRegistrar)) {
        qCDebug(DBUSMENUPROXY) << "Global menu service is running, starting right away";
        init();
    } else {
        qCDebug(DBUSMENUPROXY) << "No global menu service available, waiting for it to start before doing anything";

        // be sure when started to restore gtk menus when there's no dbus menu around in case we crashed
        enableGtkSettings(false);
    }

    // kde-gtk-config just deletes and re-creates the gtkrc-2.0, watch this and add our config to it again
    m_writeGtk2SettingsTimer->setSingleShot(true);
    m_writeGtk2SettingsTimer->setInterval(1000);
    connect(m_writeGtk2SettingsTimer, &QTimer::timeout, this, &MenuProxy::writeGtk2Settings);

    auto startGtk2SettingsTimer = [this] {
        if (!m_writeGtk2SettingsTimer->isActive()) {
            m_writeGtk2SettingsTimer->start();
        }
    };

    connect(m_gtk2RcWatch, &KDirWatch::created, this, startGtk2SettingsTimer);
    connect(m_gtk2RcWatch, &KDirWatch::dirty, this, startGtk2SettingsTimer);
    m_gtk2RcWatch->addFile(gtkRc2Path());
}

MenuProxy::~MenuProxy()
{
    teardown();
}

bool MenuProxy::init()
{
    if (!QDBusConnection::sessionBus().registerService(s_ourServiceName)) {
        qCWarning(DBUSMENUPROXY) << "Failed to register DBus service" << s_ourServiceName;
        return false;
    }

    enableGtkSettings(true);

    connect(KWindowSystem::self(), &KWindowSystem::windowAdded, this, &MenuProxy::onWindowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this, &MenuProxy::onWindowRemoved);

    const auto windows = KWindowSystem::windows();
    for (WId id : windows) {
        onWindowAdded(id);
    }

    if (m_windows.isEmpty()) {
        qCDebug(DBUSMENUPROXY) << "Up and running but no windows with menus in sight";
    }

    return true;
}

void MenuProxy::teardown()
{
    enableGtkSettings(false);

    QDBusConnection::sessionBus().unregisterService(s_ourServiceName);

    disconnect(KWindowSystem::self(), &KWindowSystem::windowAdded, this, &MenuProxy::onWindowAdded);
    disconnect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this, &MenuProxy::onWindowRemoved);

    qDeleteAll(m_windows);
    m_windows.clear();
}

void MenuProxy::enableGtkSettings(bool enable)
{
    m_enabled = enable;

    writeGtk2Settings();
    writeGtk3Settings();

    // TODO use gconf/dconf directly or at least signal a change somehow?
}

QString MenuProxy::gtkRc2Path()
{
    return QDir::homePath() + QLatin1String("/.gtkrc-2.0");
}

QString MenuProxy::gtk3SettingsIniPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/gtk-3.0/settings.ini");
}

void MenuProxy::writeGtk2Settings()
{
    QFile rcFile(gtkRc2Path());
    if (!rcFile.exists()) {
        // Don't create it here, that would break writing default GTK-2.0 settings on first login,
        // as the gtkbreeze kconf_update script only does so if it does not exist
        return;
    }

    qCDebug(DBUSMENUPROXY) << "Writing gtkrc-2.0 to" << (m_enabled ? "enable" : "disable") << "global menu support";

    if (!rcFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        return;
    }

    QByteArray content;

    QStringList gtkModules;

    while (!rcFile.atEnd()) {
        const QByteArray rawLine = rcFile.readLine();

        const QString line = QString::fromUtf8(rawLine.trimmed());

        if (!line.startsWith(s_gtkModules)) {
            // keep line as-is
            content += rawLine;
            continue;
        }

        const int equalSignIdx = line.indexOf(QLatin1Char('='));
        if (equalSignIdx < 1) {
            continue;
        }

        gtkModules = line.mid(equalSignIdx + 1).split(QLatin1Char(':'), QString::SkipEmptyParts);

        break;
    }

    addOrRemoveAppMenuGtkModule(gtkModules);

    if (!gtkModules.isEmpty()) {
        content += QStringLiteral("%1=%2").arg(s_gtkModules, gtkModules.join(QLatin1Char(':'))).toUtf8();
    }

    qCDebug(DBUSMENUPROXY) << "  gtk-modules:" << gtkModules;

    m_gtk2RcWatch->stopScan();

    // now write the new contents of the file
    rcFile.resize(0);
    rcFile.write(content);
    rcFile.close();

    m_gtk2RcWatch->startScan();
}

void MenuProxy::writeGtk3Settings()
{
    qCDebug(DBUSMENUPROXY) << "Writing gtk-3.0/settings.ini" << (m_enabled ? "enable" : "disable") << "global menu support";

    // mostly taken from kde-gtk-config
    auto cfg = KSharedConfig::openConfig(gtk3SettingsIniPath(), KConfig::NoGlobals);
    KConfigGroup group(cfg, "Settings");

    QStringList gtkModules = group.readEntry("gtk-modules", QString()).split(QLatin1Char(':'), QString::SkipEmptyParts);
    addOrRemoveAppMenuGtkModule(gtkModules);

    if (!gtkModules.isEmpty()) {
        group.writeEntry("gtk-modules", gtkModules.join(QLatin1Char(':')));
    } else {
        group.deleteEntry("gtk-modules");
    }

    qCDebug(DBUSMENUPROXY) << "  gtk-modules:" << gtkModules;

    if (m_enabled) {
        group.writeEntry("gtk-shell-shows-menubar", 1);
    } else {
        group.deleteEntry("gtk-shell-shows-menubar");
    }

    qCDebug(DBUSMENUPROXY) << "  gtk-shell-shows-menubar:" << (m_enabled ? 1 : 0);

    group.sync();
}

void MenuProxy::addOrRemoveAppMenuGtkModule(QStringList &list)
{
    if (m_enabled && !list.contains(s_appMenuGtkModule)) {
        list.append(s_appMenuGtkModule);
    } else if (!m_enabled) {
        list.removeAll(s_appMenuGtkModule);
    }
}

void MenuProxy::onWindowAdded(WId id)
{
    if (m_windows.contains(id)) {
        return;
    }

    KWindowInfo info(id, NET::WMWindowType);

    NET::WindowType wType = info.windowType(NET::NormalMask | NET::DesktopMask | NET::DockMask | NET::ToolbarMask | NET::MenuMask | NET::DialogMask
                                            | NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask);

    // Only top level windows typically have a menu bar, dialogs, such as settings don't
    if (wType != NET::Normal) {
        qCDebug(DBUSMENUPROXY) << "Ignoring window" << id << "of type" << wType;
        return;
    }

    const QString serviceName = QString::fromUtf8(getWindowPropertyString(id, s_gtkUniqueBusName));

    if (serviceName.isEmpty()) {
        return;
    }

    const QString applicationObjectPath = QString::fromUtf8(getWindowPropertyString(id, s_gtkApplicationObjectPath));
    const QString unityObjectPath = QString::fromUtf8(getWindowPropertyString(id, s_unityObjectPath));
    const QString windowObjectPath = QString::fromUtf8(getWindowPropertyString(id, s_gtkWindowObjectPath));

    const QString applicationMenuObjectPath = QString::fromUtf8(getWindowPropertyString(id, s_gtkAppMenuObjectPath));
    const QString menuBarObjectPath = QString::fromUtf8(getWindowPropertyString(id, s_gtkMenuBarObjectPath));

    if (applicationMenuObjectPath.isEmpty() && menuBarObjectPath.isEmpty()) {
        return;
    }

    Window *window = new Window(serviceName);
    window->setWinId(id);
    window->setApplicationObjectPath(applicationObjectPath);
    window->setUnityObjectPath(unityObjectPath);
    window->setWindowObjectPath(windowObjectPath);
    window->setApplicationMenuObjectPath(applicationMenuObjectPath);
    window->setMenuBarObjectPath(menuBarObjectPath);
    m_windows.insert(id, window);

    connect(window, &Window::requestWriteWindowProperties, this, [this, window] {
        Q_ASSERT(!window->proxyObjectPath().isEmpty());

        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuServiceName, s_ourServiceName.toUtf8());
        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuObjectPath, window->proxyObjectPath().toUtf8());
    });
    connect(window, &Window::requestRemoveWindowProperties, this, [this, window] {
        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuServiceName, QByteArray());
        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuObjectPath, QByteArray());
    });

    window->init();
}

void MenuProxy::onWindowRemoved(WId id)
{
    // no need to cleanup() (which removes window properties) when the window is gone, delete right away
    delete m_windows.take(id);
}

QByteArray MenuProxy::getWindowPropertyString(WId id, const QByteArray &name)
{
    QByteArray value;

    auto atom = getAtom(name);
    if (atom == XCB_ATOM_NONE) {
        return value;
    }

    // GTK properties aren't XCB_ATOM_STRING but a custom one
    auto utf8StringAtom = getAtom(QByteArrayLiteral("UTF8_STRING"));

    static const long MAX_PROP_SIZE = 10000;
    auto propertyCookie = xcb_get_property(m_xConnection, false, id, atom, utf8StringAtom, 0, MAX_PROP_SIZE);
    QScopedPointer<xcb_get_property_reply_t, QScopedPointerPodDeleter> propertyReply(xcb_get_property_reply(m_xConnection, propertyCookie, nullptr));
    if (propertyReply.isNull()) {
        qCWarning(DBUSMENUPROXY) << "XCB property reply for atom" << name << "on" << id << "was null";
        return value;
    }

    if (propertyReply->type == utf8StringAtom && propertyReply->format == 8 && propertyReply->value_len > 0) {
        const char *data = (const char *)xcb_get_property_value(propertyReply.data());
        int len = propertyReply->value_len;
        if (data) {
            value = QByteArray(data, data[len - 1] ? len : len - 1);
        }
    }

    return value;
}

void MenuProxy::writeWindowProperty(WId id, const QByteArray &name, const QByteArray &value)
{
    auto atom = getAtom(name);
    if (atom == XCB_ATOM_NONE) {
        return;
    }

    if (value.isEmpty()) {
        xcb_delete_property(m_xConnection, id, atom);
    } else {
        xcb_change_property(m_xConnection, XCB_PROP_MODE_REPLACE, id, atom, XCB_ATOM_STRING, 8, value.length(), value.constData());
    }
}

xcb_atom_t MenuProxy::getAtom(const QByteArray &name)
{
    static QHash<QByteArray, xcb_atom_t> s_atoms;

    auto atom = s_atoms.value(name, XCB_ATOM_NONE);
    if (atom == XCB_ATOM_NONE) {
        const xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom(m_xConnection, false, name.length(), name.constData());
        QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> atomReply(xcb_intern_atom_reply(m_xConnection, atomCookie, nullptr));
        if (!atomReply.isNull()) {
            atom = atomReply->atom;
            if (atom != XCB_ATOM_NONE) {
                s_atoms.insert(name, atom);
            }
        }
    }

    return atom;
}
