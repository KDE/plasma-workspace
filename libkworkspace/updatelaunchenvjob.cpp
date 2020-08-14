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

    static bool isSystemdApprovedValue(const QString &value);

    UpdateLaunchEnvJob *q;
    QProcessEnvironment environment;
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
    d->environment.insert(varName, value);
    start();
}

UpdateLaunchEnvJob::UpdateLaunchEnvJob(const QProcessEnvironment &environment)
    : d(new Private(this))
{
    d->environment = environment;
    start();
}

UpdateLaunchEnvJob::~UpdateLaunchEnvJob()
{
    delete d;
}

void UpdateLaunchEnvJob::start()
{
    qDBusRegisterMetaType<QMap<QString, QString>>();
    QMap<QString, QString> dbusActivationEnv;
    QStringList systemdUpdates;

    for (const auto &varName : d->environment.keys()) {
        const QString value = d->environment.value(varName);

        // KLauncher
        org::kde::KLauncher klauncher(QStringLiteral("org.kde.klauncher5"),
                                    QStringLiteral("/KLauncher"),
                                    QDBusConnection::sessionBus());
        auto klauncherReply = klauncher.setLaunchEnv(varName, value);
        d->monitorReply(klauncherReply);

        // plasma-session
        org::kde::Startup startup(QStringLiteral("org.kde.Startup"),
                                QStringLiteral("/Startup"),
                                QDBusConnection::sessionBus());
        auto startupReply = startup.updateLaunchEnv(varName, value);
        d->monitorReply(startupReply);


        // DBus-activation environment
        dbusActivationEnv.insert(varName, value);

        // _user_ systemd env
        // Systemd has stricter parsing of valid environment variables
        // https://github.com/systemd/systemd/issues/16704
        // validate here
        if (!Private::isSystemdApprovedValue(value)) {
            qWarning() << "Skipping syncing of environment variable " << varName << "as value contains unsupported characters";
            continue;
        }
        const QString updateString = varName + QStringLiteral("=") + value;
        systemdUpdates.append(updateString);
    }

    // DBus-activation environment
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
    systemdActivationMsg.setArguments({systemdUpdates});


    auto systemdActivationReply = QDBusConnection::sessionBus().asyncCall(systemdActivationMsg);
    d->monitorReply(systemdActivationReply);
}

bool UpdateLaunchEnvJob::Private::isSystemdApprovedValue(const QString &value)
{
    // systemd code checks that a value contains no control characters except \n \t
    // effectively copied from systemd's string_has_cc
    for (const char &it : value.toLatin1()) {
        if (it == QLatin1Char('\n') || it == QLatin1Char('\t')) {
            continue;
        }
        if (it > 0 && it < ' ') {
            return false;
        }
        if (it == 127) {
            return false;
        }
    }
    return true;
}
