/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencastingrequest.h"
#include "logging.h"

#include <QCoreApplication>
#include <QDebug>

ScreencastingRequest::ScreencastingRequest(QObject *parent)
    : QObject(parent)
{
}

ScreencastingRequest::~ScreencastingRequest() = default;

quint32 ScreencastingRequest::nodeId() const
{
    return m_nodeId;
}

void ScreencastingRequest::resetUuid()
{
    setUuid(QString());
}

void ScreencastingRequest::setUuid(const QString &uuid)
{
    if (m_uuid == uuid) {
        return;
    }

    setStream(nullptr);
    m_uuid = uuid;
    Q_EMIT uuidChanged(uuid);

    if (!m_uuid.isEmpty()) {
        if (!m_screenCasting) {
            m_screenCasting = std::make_unique<Screencasting>();
        }
        setStream(m_screenCasting->createWindowStream(m_uuid, Screencasting::pointer_hidden));
    }
}

void ScreencastingRequest::resetOutputName()
{
    setOutputName(QString());
}

void ScreencastingRequest::setOutputName(const QString &outputName)
{
    if (m_outputName == outputName) {
        return;
    }

    setStream(nullptr);
    m_outputName = outputName;
    Q_EMIT outputNameChanged(outputName);

    if (!m_outputName.isEmpty()) {
        if (!m_screenCasting) {
            m_screenCasting = std::make_unique<Screencasting>();
        }
        setStream(m_screenCasting->createOutputStream(m_outputName, Screencasting::pointer_hidden));
    }
}

void ScreencastingRequest::setStream(std::unique_ptr<ScreencastingStream> stream)
{
    if (stream) {
        m_stream = std::move(stream);

        connect(m_stream.get(), &ScreencastingStream::created, this, &ScreencastingRequest::setNodeid);
        connect(m_stream.get(), &ScreencastingStream::closed, this, [this]() {
            setNodeid(0);
        });
        connect(m_stream.get(), &ScreencastingStream::failed, this, [](const QString &error) {
            qWarning() << "error creating screencast" << error;
        });
    } else {
        m_stream.reset();
        setNodeid(0);
    }
}

void ScreencastingRequest::setNodeid(uint nodeId)
{
    if (nodeId != m_nodeId) {
        m_nodeId = nodeId;
        Q_EMIT nodeIdChanged(nodeId);
    }
}

QString ScreencastingRequest::uuid() const
{
    return m_uuid;
}

QString ScreencastingRequest::outputName() const
{
    return m_outputName;
}
