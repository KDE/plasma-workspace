/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

 Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
 Copyright (C) 2002 Luboš Luňák <l.lunak@kde.org>
 Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
 Copyright (C) 2008 Chani Armitage <chanika@gmail.com>
 Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>
 Copyright (C) 2015 Bhushan Shah <bhush94@gmail.com>

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

#ifndef ABSTRACTLOCKER_H
#define ABSTRACTLOCKER_H

#include <QObject>
#include <QRasterWindow>

class GlobalAccel;

namespace ScreenLocker
{

class AbstractLocker;

class BackgroundWindow : public QRasterWindow
{
    Q_OBJECT
public:
    explicit BackgroundWindow(AbstractLocker *lock);
    virtual ~BackgroundWindow();

    void emergencyShow();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    AbstractLocker *m_lock;
    bool m_greeterFailure = false;
};

class AbstractLocker : public QObject
{
    Q_OBJECT
public:
    AbstractLocker();
    virtual ~AbstractLocker();

    virtual void showLockWindow() = 0;
    virtual void hideLockWindow() = 0;

    virtual void addAllowedWindow(quint32 window);

    void setGlobalAccel(GlobalAccel *ga) {
        m_globalAccel = ga;
    }

    void emergencyShow();

Q_SIGNALS:
    void userActivity();
    void lockWindowShown();

protected:

    virtual void stayOnTop() = 0;

    GlobalAccel* globalAccel() {
        return m_globalAccel;
    }
    QScopedPointer<BackgroundWindow> m_background;

private:
    GlobalAccel *m_globalAccel = nullptr;

    friend class BackgroundWindow;

};

}

#endif //ABSTRACTLOCKER_H
