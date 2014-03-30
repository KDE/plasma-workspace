/*
 * Copyright 2012 Gregor Taetzner gregor@freenet.de
 *
 * This program is free software; you can redistribute it and/or        *
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

#include "packagekitengine.h"
#include "packagekitservice.h"

#include <QDBusMessage>
#include <QDBusConnection>

PackagekitEngine::PackagekitEngine(QObject* parent, const QVariantList& args)
    : DataEngine(parent, args),
    m_pk_available(false)
{
}

void PackagekitEngine::init()
{
    QDBusMessage message;
    message = QDBusMessage::createMethodCall("org.freedesktop.DBus",
                                             "/org/freedesktop/DBus",
                                             "org.freedesktop.DBus",
                                             "ListActivatableNames");

    QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage
        && reply.arguments().size() == 1) {
        QStringList list = reply.arguments().first().toStringList();
        if (list.contains("org.freedesktop.PackageKit")) {
            m_pk_available = true;
        }
    }

    setData("Status", "available", m_pk_available);
}

Plasma::Service* PackagekitEngine::serviceForSource(const QString& source)
{
    if (m_pk_available) {
        return new PackagekitService(this);
    }

    // if packagekit not available, return null service
    return Plasma::DataEngine::serviceForSource(source);
}

K_EXPORT_PLASMA_DATAENGINE(packagekit, PackagekitEngine)

#include "packagekitengine.moc"