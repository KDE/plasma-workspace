/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>
#include "screencasting.h"

class ScreencastingStream;

/**
 * Allows us to request a stream for a window identified by its universally
 * unique identifier.
 *
 * We will get a PipeWire node id that can be fed to any pipewire player, be it
 * the PipeWireSourceItem, GStreamer's pipewiresink or any other.
 */
class ScreencastingRequest : public QObject
{
    Q_OBJECT
    /**
     * The unique identifier of the window we want to cast.
     *
     * @see PlasmaWindow::uuid
     * @see PlasmaWindow::stackingOrderUuids
     * @see PlasmaWindowModel::Uuid
     * @see TasksModel::WinIdList
     */
    Q_PROPERTY(QString uuid READ uuid WRITE setUuid NOTIFY uuidChanged)

    /** The offered nodeId to give to a source */
    Q_PROPERTY(quint32 nodeId READ nodeId NOTIFY nodeIdChanged)
public:
    ScreencastingRequest(QObject* parent = nullptr);
    ~ScreencastingRequest();

    void setUuid(const QString &uuid);
    QString uuid() const;
    quint32 nodeId() const;

    void create(Screencasting* screencasting);

Q_SIGNALS:
    void nodeIdChanged(quint32 nodeId);
    void uuidChanged(const QString &uuid);
    void closeRunningStreams();
    void cursorModeChanged(Screencasting::CursorMode cursorMode);

private:
    void setNodeid(uint nodeId);

    ScreencastingStream* m_stream = nullptr;
    QString m_uuid;
    KWayland::Client::Output* m_output = nullptr;
    quint32 m_nodeId = 0;
};
