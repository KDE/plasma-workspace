/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencastingrequest.h"
#include "logging.h"

#include <QCoreApplication>
#include <QDebug>
#include <QPointer>
#include <functional>

struct ScreencastingRequestPrivate {
    Screencasting *m_screenCasting = nullptr;
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

    setNodeid(0);
    d->m_uuid = uuid;
    Q_EMIT uuidChanged(uuid);

    if (!d->m_uuid.isEmpty()) {
        if (!d->m_screenCasting) {
            d->m_screenCasting = new Screencasting(this);
        }
        auto stream = d->m_screenCasting->createWindowStream(d->m_uuid, Screencasting::CursorMode::Hidden);
        if (!stream) {
            return;
        }
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
        if (!d->m_screenCasting) {
            d->m_screenCasting = new Screencasting(this);
        }
        auto stream = d->m_screenCasting->createOutputStream(d->m_outputName, Screencasting::CursorMode::Hidden);
        if (!stream) {
            return;
        }
        adopt(stream);
        stream->setObjectName(d->m_outputName);
    }
}

void ScreencastingRequest::adopt(ScreencastingStream *stream)
{
    d->m_stream = stream;

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
    if (nodeId != d->m_nodeId) {
        d->m_nodeId = nodeId;
        Q_EMIT nodeIdChanged(nodeId);
    }

    if (nodeId == 0 && d->m_stream) {
        delete d->m_stream;
    }
}

QString ScreencastingRequest::uuid() const
{
    return d->m_uuid;
}

QString ScreencastingRequest::outputName() const
{
    return d->m_outputName;
}
