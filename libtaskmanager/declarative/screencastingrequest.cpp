/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencastingrequest.h"
#include "logging.h"

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <QCoreApplication>
#include <QDebug>
#include <QPointer>
#include <functional>

struct ScreencastingRequestPrivate {
    QPointer<ScreencastingStream> m_stream;
    QString m_uuid;
    QString m_outputName;
    quint32 m_nodeId = 0;
};

ScreencastingRequest::ScreencastingRequest(QObject *parent)
    : QObject(parent)
    , d(new ScreencastingRequestPrivate)
{
}

ScreencastingRequest::~ScreencastingRequest() = default;

quint32 ScreencastingRequest::nodeId() const
{
    return d->m_nodeId;
}

void ScreencastingRequest::setUuid(const QString &uuid)
{
    if (d->m_uuid == uuid) {
        return;
    }

    d->m_stream->deleteLater();
    setNodeid(0);

    d->m_uuid = uuid;
    Q_EMIT uuidChanged(uuid);

    if (!d->m_uuid.isEmpty()) {
        auto screencasting = new Screencasting(this);
        auto stream = screencasting->createWindowStream(d->m_uuid, Screencasting::CursorMode::Hidden);
        adopt(stream);
    }
}

void ScreencastingRequest::setOutputName(const QString &outputName)
{
    if (d->m_outputName == outputName) {
        return;
    }

    setNodeid(0);
    d->m_outputName = outputName;
    Q_EMIT outputNameChanged(outputName);

    if (!d->m_outputName.isEmpty()) {
        auto screencasting = new Screencasting(this);
        auto stream = screencasting->createOutputStream(d->m_outputName, Screencasting::CursorMode::Hidden);
        adopt(stream);
        stream->setObjectName(d->m_outputName);
    }
}

void ScreencastingRequest::adopt(ScreencastingStream *stream)
{
    connect(stream, &ScreencastingStream::created, this, &ScreencastingRequest::setNodeid);
    connect(stream, &ScreencastingStream::failed, this, [](const QString &error) {
        qWarning() << "error creating screencast" << error;
    });
    connect(stream, &ScreencastingStream::closed, this, [this, stream] {
        if (stream->nodeId() == d->m_nodeId) {
            setNodeid(0);
        }
    });
}

void ScreencastingRequest::setNodeid(uint nodeId)
{
    if (nodeId == d->m_nodeId) {
        return;
    }

    d->m_nodeId = nodeId;
    Q_EMIT nodeIdChanged(nodeId);
}

QString ScreencastingRequest::uuid() const
{
    return d->m_uuid;
}

QString ScreencastingRequest::outputName() const
{
    return d->m_outputName;
}
