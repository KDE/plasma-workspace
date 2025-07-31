/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>

#include <KIO/FileSystemFreeSpaceJob>

#include "stateinfo.h"
#include "storageinfo.h"

class SpaceUpdateMonitor;

/**
 * This class monitors the full and free size of devices
 */
class SpaceInfo : public QObject
{
    Q_OBJECT

public:
    explicit SpaceInfo(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent = nullptr);
    ~SpaceInfo() override;

    void updateSize();

    std::optional<double> getFullSize() const;
    std::optional<double> getFreeSize() const;

private Q_SLOTS:
    void onStateChanged();
    void onSpaceUpdated(KJob *job);
    void onDeviceChanged(const QMap<QString, int> &props);

Q_SIGNALS:
    void sizeChanged(const QString &udi);

private:
    std::optional<double> m_fullSize;
    std::optional<double> m_freeSize;

    KIO::FileSystemFreeSpaceJob *m_job;

    std::shared_ptr<StorageInfo> m_storageInfo;
    std::shared_ptr<StateInfo> m_stateInfo;
    std::shared_ptr<SpaceUpdateMonitor> m_spaceUpdateMonitor;
};
