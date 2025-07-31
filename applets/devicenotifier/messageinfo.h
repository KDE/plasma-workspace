/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QObject>

#include <Solid/SolidNamespace>

#include "stateinfo.h"
#include "storageinfo.h"

/**
 * Class that monitors messages for devices
 */
class MessageInfo : public QObject
{
    Q_OBJECT

public:
    explicit MessageInfo(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent = nullptr);

    QString getMessage() const;

Q_SIGNALS:
    void messageChanged(const QString &udi);

    void blockingAppsReady(const QStringList &apps);

private Q_SLOTS:
    void onStateChanged();
    void notify(const std::optional<QString> &message);
    void queryBlockingApps(const QString &devicePath);
    void clearPreviousMessage();

private:
    QString m_message;

    std::shared_ptr<StorageInfo> m_storageInfo;
    std::shared_ptr<StateInfo> m_stateInfo;
};
