/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "request.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QFile>

using namespace Qt::StringLiterals;

Request::Request(const QDBusObjectPath &handle, QObject *parent)
    : QDBusVirtualObject(parent)
    , m_handle(handle)
{
    auto sessionBus = QDBusConnection::sessionBus();
    if (!sessionBus.registerVirtualObject(handle.path(), this, QDBusConnection::VirtualObjectRegisterOption::SubPath)) {
        qWarning() << sessionBus.lastError().message();
        qWarning() << "Failed to register request object for" << handle.path();
        deleteLater();
    }
}

Request::~Request()
{
}

void Request::sendCancel()
{
    qWarning() << "Sending Cancel for" << m_handle.path();
    QDBusMessage message = QDBusMessage::createSignal(m_handle.path(), u"org.kde.secretprompter.request"_s, u"Dismiss"_s);
    QDBusConnection::sessionBus().send(message);
    QDBusConnection::sessionBus().unregisterObject(m_handle.path());
}

bool Request::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    /* Check to make sure we're getting properties on our interface */
    if (message.type() != QDBusMessage::MessageType::MethodCallMessage) {
        return false;
    }

    qWarning() << message.interface();
    qWarning() << message.member();
    qWarning() << message.path();

    if (message.interface() != QLatin1String("org.kde.secretprompter.request")) {
        return false;
    }

    if (message.member() == QLatin1String("Result")) {
        Q_ASSERT(message.arguments().length() == 2);
        qWarning() << "Result type:" << message;

        auto dbusFd = message.arguments()[1].value<QDBusUnixFileDescriptor>();

        QFile readFile;
        if (!readFile.open(dbusFd.fileDescriptor(), QIODevice::ReadOnly)) {
            qWarning() << "Failed to open read pipe fd as QFile for prompt result:" << readFile.errorString();
            return false;
        }

        qDebug() << "Received secret:" << readFile.readAll();

        connection.send(message.createReply());
    }

    deleteLater();

    return true;
}

QString Request::introspect(const QString &path) const
{
    QString nodes;

    if (path == m_handle.path()) {
        nodes = QStringLiteral(
            "<interface name=\"org.kde.secretprompter.request\">"
            "   <method name=\"Result\">"
            "       <arg name=\"type\" type=\"s\" direction=\"in\"/>"
            "       <arg name=\"reply\" type=\"h\" direction=\"in\"/>"
            "   </method>"
            "   <signal name=\"Dismiss\">"
            "   </signal>"
            "</interface>");
    }

    return nodes;
}

#include "moc_request.cpp"
