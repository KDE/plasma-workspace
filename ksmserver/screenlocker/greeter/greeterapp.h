/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

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
#ifndef SCREENLOCKER_GREETERAPP_H
#define SCREENLOCKER_GREETERAPP_H

#include <QApplication>

#include <Plasma/PackageStructure>
#include <QQuickView>

namespace Plasma {
    class Package;
};

class Authenticator;

namespace ScreenLocker
{
class Unlocker;

class UnlockApp : public QApplication
{
    Q_OBJECT
public:
    explicit UnlockApp(int &argc, char **argv);
    virtual ~UnlockApp();

    void setTesting(bool enable);
    void setImmediateLock(bool immediateLock);
    void lockImmediately();

public Q_SLOTS:
    void desktopResized();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void viewStatusChanged(const QQuickView::Status &status);
    void resetRequestIgnore();
    void suspendToRam();
    void suspendToDisk();
    void shutdown();
    void getFocus();
    void setLockedPropertyOnViews();

private:
    void initialize();
    void capsLocked();
    void shareEvent(QEvent *e, QQuickView *from);

    QString m_mainQmlPath;
    QList<QQuickView*> m_views;
    QTimer *m_resetRequestIgnoreTimer;
    QTimer *m_delayedLockTimer;
    Plasma::Package m_package;
    bool m_testing;
    bool m_capsLocked;
    bool m_ignoreRequests;
    bool m_immediateLock;
    bool m_runtimeInitialized;
    Authenticator *m_authenticator;
};
} // namespace

#endif // SCREENLOCKER_GREETERAPP_H
