/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include "systemclipboard.h"
#include <memory>

class DataControlDevice;
class DataControlDeviceManager;
class WlQueue;

class WaylandClipboard : public SystemClipboard
{
public:
    WaylandClipboard(QObject *parent);
    ~WaylandClipboard() override;
    void setMimeData(QMimeData *mime, QClipboard::Mode mode) override;
    void clear(QClipboard::Mode mode) override;
    const QMimeData *mimeData(QClipboard::Mode mode) const override;

private:
    std::unique_ptr<DataControlDeviceManager> m_manager;
    std::unique_ptr<DataControlDevice> m_device;
    QThread *m_sourceThread;
    WlQueue *m_sourceQueue;
};
