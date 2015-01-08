/*
 * Copyright 2014  Martin Graesslin <mgraesslin@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QProcessEnvironment>
#include <QDebug>

typedef QMap<QString,QString> EnvMap;
Q_DECLARE_METATYPE(EnvMap)

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    EnvMap envMap;
    for (const QString &key : env.keys()) {
        envMap.insert(key, env.value(key));
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                                      QStringLiteral("/org/freedesktop/DBus"),
                                                      QStringLiteral("org.freedesktop.DBus"),
                                                      QStringLiteral("UpdateActivationEnvironment"));
    qDBusRegisterMetaType<EnvMap>();
    msg.setArguments(QList<QVariant>({QVariant::fromValue(envMap)}));

    QDBusPendingCall reply = QDBusConnection::sessionBus().asyncCall(msg);
    reply.waitForFinished();
    if (reply.isError()) {
	qDebug() << reply.error().name() << reply.error().message();
    }
    return reply.isError() ? 1 : 0;
}
