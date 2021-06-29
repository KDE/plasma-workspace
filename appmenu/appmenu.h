/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <config-X11.h>
#ifdef HAVE_X11
#include <xcb/xcb.h>
#endif

#include <kdedmodule.h>

#include "menuimporter.h"
#include <QPointer>

class QDBusServiceWatcher;
class KDBusMenuImporter;
class AppmenuDBus;
class VerticalMenu;
namespace KWayland
{
namespace Client
{
class PlasmaShell;
};
};

class AppMenuModule : public KDEDModule, protected QDBusContext
{
    Q_OBJECT
public:
    AppMenuModule(QObject *parent, const QList<QVariant> &list);
    ~AppMenuModule() override;
    bool eventFilter(QObject *object, QEvent *event) override;

Q_SIGNALS:
    /**
     * We do not know where is menu decoration button, so tell kwin to show menu
     */
    void showRequest(const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId);
    /**
     * This signal is emitted whenever popup menu/menubar is shown
     * Useful for decorations to know if menu button should look pressed
     */
    void menuShown(const QString &service, const QDBusObjectPath &objectPath);
    /**
     * This signal is emitted whenever popup menu/menubar is hidden
     * Useful for decorations to know if menu button should be release
     */
    void menuHidden(const QString &service, const QDBusObjectPath &objectPath);

private Q_SLOTS:
    /**
     * A new window was registered to AppMenu
     *
     * For compatibility this will set the DBus service name and menu object path as properties
     * on the window so we keep working with clients that use the DBusMenu "properly".
     */
    void slotWindowRegistered(WId id, const QString &serviceName, const QDBusObjectPath &menuObjectPath);
    /**
     * Show menu at QPoint(x,y) for DBus serviceName and menuObjectPath
     * if x or y == -1, show in application window
     */
    void slotShowMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId);
    /**
     * Reconfigure module
     */
    void reconfigure();

    void itemActivationRequested(int actionId, uint timeStamp);

private:
    void hideMenu();

    void fakeUnityAboutToShow(const QString &service, const QDBusObjectPath &menuObjectPath);

    KDBusMenuImporter *getImporter(const QString &service, const QString &path);
    void initMenuWayland();

    MenuImporter *m_menuImporter = nullptr;
    AppmenuDBus *m_appmenuDBus;
    QDBusServiceWatcher *m_menuViewWatcher;
    QPointer<VerticalMenu> m_menu;

#ifdef HAVE_X11
    xcb_connection_t *m_xcbConn = nullptr;
#endif
    KWayland::Client::PlasmaShell *m_plasmashell = nullptr;
};
