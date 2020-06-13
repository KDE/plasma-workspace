/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencastingrequest.h"
#include "screencasting.h"
#include <functional>
#include <QCoreApplication>
#include <QPointer>
#include <QDebug>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>

class ScreencastingSingleton : public QObject
{
    Q_OBJECT
public:
    ScreencastingSingleton(QObject* parent)
        : QObject(parent)
    {
        KWayland::Client::ConnectionThread *connection = KWayland::Client::ConnectionThread::fromApplication(this);
        if (!connection) {
            return;
        }

        KWayland::Client::Registry *registry = new KWayland::Client::Registry(this);

        connect(registry, &KWayland::Client::Registry::interfaceAnnounced, this, [this, registry] (const QByteArray &interfaceName, quint32 name, quint32 version) {
            if (interfaceName != "zkde_screencast_unstable_v1")
                return;

            m_screencasting = new Screencasting(registry, name, version, this);
            Q_EMIT created(m_screencasting);
        });

        registry->create(connection);
        registry->setup();
    }

    static ScreencastingSingleton* self() {
        static QPointer<ScreencastingSingleton> s_self;
        if (!s_self && QCoreApplication::instance())
            s_self = new ScreencastingSingleton(QCoreApplication::instance());
        return s_self;
    }

    void requestInterface(ScreencastingRequest* item) {
        if (!m_screencasting) {
            connect(this, &ScreencastingSingleton::created, item, &ScreencastingRequest::create, Qt::UniqueConnection);
        } else {
            item->create(m_screencasting);
        }
    }

Q_SIGNALS:
    void created(Screencasting* screencasting);

private:
    Screencasting* m_screencasting = nullptr;
};

ScreencastingRequest::ScreencastingRequest(QObject* parent)
    : QObject(parent)
{
}

ScreencastingRequest::~ScreencastingRequest() = default;

quint32 ScreencastingRequest::nodeId() const
{
    return m_nodeId;
}

void ScreencastingRequest::setUuid(const QString& uuid)
{
    if (m_uuid == uuid) {
        return;
    }

    Q_EMIT closeRunningStreams();
    setNodeid(0);

    m_uuid = uuid;
    if (!m_uuid.isEmpty()) {
        ScreencastingSingleton::self()->requestInterface(this);
    }

    Q_EMIT uuidChanged(uuid);
}

void ScreencastingRequest::setNodeid(uint nodeId)
{
    if (nodeId == m_nodeId) {
        return;
    }

    m_nodeId = nodeId;
    Q_EMIT nodeIdChanged(nodeId);
}

void ScreencastingRequest::create(Screencasting* screencasting)
{
    auto stream = screencasting->createWindowStream(m_uuid, Screencasting::CursorMode::Hidden);
    stream->setObjectName(m_uuid);

    connect(stream, &ScreencastingStream::created, this, [stream, this] (int nodeId) {
        if (stream->objectName() == m_uuid) {
            setNodeid(nodeId);
        }
    });
    connect(stream, &ScreencastingStream::failed, this, [] (const QString &error) {
        qWarning() << "error creating screencast" << error;
    });
    connect(stream, &ScreencastingStream::closed, this, [this, stream] {
        if (stream->nodeId() == m_nodeId) {
            setNodeid(0);
        }
    });
    connect(this, &ScreencastingRequest::closeRunningStreams, stream, &QObject::deleteLater);
}

QString ScreencastingRequest::uuid() const
{
    return m_uuid;
}

#include "screencastingrequest.moc"
