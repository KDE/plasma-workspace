/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

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

#ifndef APPMENU_DBUS_H
#define APPMENU_DBUS_H

// Qt
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>
#include <QDebug>
#include <qwindowdefs.h>

class KDBusMenuImporter;

class AppmenuDBus : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    AppmenuDBus(QObject*);
    ~AppmenuDBus();

    bool connectToBus(const QString& service = QString(), const QString& path = QString());

    /**
     * DBus method showing menu at QPoint(x,y) for id
     * if x or y == -1, show in application window
     */
    void showMenu(int x, int y, WId id);
    /**
     * DBus method reconfiguring kded module
     */
    void reconfigure();

Q_SIGNALS:
    /**
     * This signal is emitted on showMenu() request
     */
    void appShowMenu(int x, int y, WId id);
    /**
     * This signal is emitted on reconfigure() request
     */
    void moduleReconfigure();

    // Dbus signals
    /**
     * This signal is emitted whenever kded want to show menu
     * We do not know where is menu decoration button, so tell kwin to show menu
     */
    void showRequest(qulonglong);
    /**
     *  This signal is emitted whenever application menu becomes available
     */
    void menuAvailable(qulonglong);
    /**
     * This signal is emitted whenever menus are unavailables
     */
    void clearMenus();
    /**
     * This signal is emitted whenever popup menu/menubar is hidden
     * Useful for decorations to know if menu button should be release
     */
    void menuHidden(qulonglong);
    /**
     * This signal is emitted whenever a window register to appmenu
     */
    void WindowRegistered(qulonglong wid, const QString& service, const QDBusObjectPath&);
    /**
     * This signal is emitted whenever a window unregister from appmenu
     */
    void WindowUnregistered(qulonglong wid);

private:
    QString m_service;
};

#endif // APPMENU_DBUS_H
