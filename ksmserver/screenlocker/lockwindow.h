/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
Copyright (C) 2002 Luboš Luňák <l.lunak@kde.org>
Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
Copyright (C) 2008 Chani Armitage <chanika@gmail.com>
Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef SCREENLOCKER_LOCKWINDOW_H
#define SCREENLOCKER_LOCKWINDOW_H
#include <QAbstractNativeEventFilter>
#include <QWidget>
#include <X11/Xlib.h>
#include <fixx11h.h>

class QTimer;
namespace ScreenLocker
{
class LockWindow : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    LockWindow();
    virtual ~LockWindow();

    void showLockWindow();
    void hideLockWindow();

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

Q_SIGNALS:
    void userActivity();

protected:
    virtual void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void autoLogoutTimeout();
    void updateGeo();

private:
    void initialize();
    void saveVRoot();
    void setVRoot(Window win, Window vr);
    void removeVRoot(Window win);
    int findWindowInfo(Window w);
    void stayOnTop();
    bool isLockWindow(Window w);
    struct WindowInfo
    {
        Window window;
        bool viewable;
    };
    QList<WindowInfo> m_windowInfo;
    QList<WId> m_lockWindows;
    QTimer *m_autoLogoutTimer;
};
}

#endif // SCREENLOCKER_LOCKWINDOW_H
