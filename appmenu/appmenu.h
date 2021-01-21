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

#ifndef APPMENUMODULE_H
#define APPMENUMODULE_H

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

class AppMenuModule : public KDEDModule, protected QDBusContext
{
    Q_OBJECT
public:
    AppMenuModule(QObject *parent, const QList<QVariant> &list);
    ~AppMenuModule() override;

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

    MenuImporter *m_menuImporter = nullptr;
    AppmenuDBus *m_appmenuDBus;
    QDBusServiceWatcher *m_menuViewWatcher;
    QPointer<VerticalMenu> m_menu;

#ifdef HAVE_X11
    xcb_connection_t *m_xcbConn = nullptr;
#endif
};

#endif
