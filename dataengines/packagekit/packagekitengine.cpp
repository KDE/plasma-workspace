/*
    SPDX-FileCopyrightText: 2012 Gregor Taetzner <gregor@freenet.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "packagekitengine.h"
#include "packagekitservice.h"

#include <QDBusConnection>
#include <QDBusMessage>

PackagekitEngine::PackagekitEngine(QObject *parent)
    : DataEngine(parent)
    , m_pk_available(false)
{
}

void PackagekitEngine::init()
{
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                             QStringLiteral("/org/freedesktop/DBus"),
                                             QStringLiteral("org.freedesktop.DBus"),
                                             QStringLiteral("ListActivatableNames"));

    QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() == 1) {
        QStringList list = reply.arguments().first().toStringList();
        if (list.contains(QLatin1String("org.freedesktop.PackageKit"))) {
            m_pk_available = true;
        }
    }

    setData(QStringLiteral("Status"), QStringLiteral("available"), m_pk_available);
}

Plasma5Support::Service *PackagekitEngine::serviceForSource(const QString &source)
{
    if (m_pk_available) {
        return new PackagekitService(this);
    }

    // if packagekit not available, return null service
    return Plasma5Support::DataEngine::serviceForSource(source);
}

K_PLUGIN_CLASS_WITH_JSON(PackagekitEngine, "plasma-dataengine-packagekit.json")

#include "packagekitengine.moc"
