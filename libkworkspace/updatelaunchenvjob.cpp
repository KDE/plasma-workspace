/*
   Copyright (C) 2020 Kai Uwe Broulik <kde@broulik.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "updatelaunchenvjob.h"

#include <klauncher_interface.h>
#include <startup_interface.h>

class Q_DECL_HIDDEN UpdateLaunchEnvJob::Private
{
public:
    explicit Private(UpdateLaunchEnvJob *q);
    void monitorReply(const QDBusPendingReply<> &reply);

    UpdateLaunchEnvJob *q;
    QString varName;
    QString value;
    int pendingReplies = 0;
};

UpdateLaunchEnvJob::Private::Private(UpdateLaunchEnvJob *q)
    : q(q)
{

}

void UpdateLaunchEnvJob::Private::monitorReply(const QDBusPendingReply<> &reply)
{
    ++pendingReplies;

    auto *watcher = new QDBusPendingCallWatcher(reply, q);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, q, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        --pendingReplies;

        if (pendingReplies == 0) {
            q->emitResult();
        }
    });
}

UpdateLaunchEnvJob::UpdateLaunchEnvJob(const QString &varName, const QString &value)
    : d(new Private(this))
{
    d->varName = varName;
    d->value = value;
    start();
}

UpdateLaunchEnvJob::~UpdateLaunchEnvJob()
{
    delete d;
}

void UpdateLaunchEnvJob::start()
{
    // KLauncher
    org::kde::KLauncher klauncher(QStringLiteral("org.kde.klauncher5"),
                                  QStringLiteral("/KLauncher"),
                                  QDBusConnection::sessionBus());
    auto klauncherReply = klauncher.setLaunchEnv(d->varName, d->value);
    d->monitorReply(klauncherReply);

    // plasma-session
    org::kde::Startup startup(QStringLiteral("org.kde.Startup"),
                              QStringLiteral("/Startup"),
                              QDBusConnection::sessionBus());
    auto startupReply = startup.updateLaunchEnv(d->varName, d->value);
    d->monitorReply(startupReply);

    // DBus-activation environment
    qDBusRegisterMetaType<QMap<QString, QString>>();
    const QMap<QString, QString> dbusActivationEnv{
        {d->varName, d->value}
    };

    QDBusMessage dbusActivationMsg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                                                    QStringLiteral("/org/freedesktop/DBus"),
                                                                    QStringLiteral("org.freedesktop.DBus"),
                                                                    QStringLiteral("UpdateActivationEnvironment"));
    dbusActivationMsg.setArguments({QVariant::fromValue(dbusActivationEnv)});

    auto dbusActivationReply = QDBusConnection::sessionBus().asyncCall(dbusActivationMsg);
    d->monitorReply(dbusActivationReply);

    // _user_ systemd env
    QDBusMessage systemdActivationMsg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                                       QStringLiteral("/org/freedesktop/systemd1"),
                                                                       QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                                       QStringLiteral("SetEnvironment"));
    const QString updateString = d->varName + QStringLiteral("=") + d->value;
    systemdActivationMsg.setArguments({QVariant(QStringList{updateString})});

    auto systemdActivationReply = QDBusConnection::sessionBus().asyncCall(systemdActivationMsg);
    d->monitorReply(systemdActivationReply);
}

