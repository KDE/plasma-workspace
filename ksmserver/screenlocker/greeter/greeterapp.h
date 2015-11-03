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

#include <QGuiApplication>

#include <KPackage/PackageStructure>
#include <QQmlComponent>

namespace KWayland {
namespace Client {
    class ConnectionThread;
    class Registry;
}
}

namespace KQuickAddons {
class QuickViewSharedEngine;
}

class Authenticator;

struct org_kde_ksld;

namespace ScreenLocker
{
class Unlocker;

class UnlockApp : public QGuiApplication
{
    Q_OBJECT
public:
    explicit UnlockApp(int &argc, char **argv);
    virtual ~UnlockApp();

    void setTesting(bool enable);
    void setImmediateLock(bool immediateLock);
    void lockImmediately();
    void setGraceTime(int milliseconds);
    void setNoLock(bool noLock);
    void setKsldSocket(int socket);

    void osdProgress(const QString &icon, int percent, const QString &additionalText);
    void osdText(const QString &icon, const QString &additionalText);

public Q_SLOTS:
    void desktopResized();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void resetRequestIgnore();
    void suspendToRam();
    void suspendToDisk();
    void getFocus();
    void markViewsAsVisible(KQuickAddons::QuickViewSharedEngine *view);
    void setLockedPropertyOnViews();

private:
    void initialize();
    void shareEvent(QEvent *e, KQuickAddons::QuickViewSharedEngine *from);

    QUrl m_mainQmlPath;
    QList<KQuickAddons::QuickViewSharedEngine*> m_views;
    QTimer *m_resetRequestIgnoreTimer;
    QTimer *m_delayedLockTimer;
    KPackage::Package m_package;
    bool m_testing;
    bool m_ignoreRequests;
    bool m_immediateLock;
    bool m_runtimeInitialized;
    Authenticator *m_authenticator;
    int m_graceTime;
    bool m_noLock;

    KWayland::Client::ConnectionThread *m_ksldConnection = nullptr;
    KWayland::Client::Registry *m_ksldRegistry = nullptr;
    QThread *m_ksldConnectionThread = nullptr;
    org_kde_ksld *m_ksldInterface = nullptr;
};
} // namespace

#endif // SCREENLOCKER_GREETERAPP_H
