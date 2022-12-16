/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KConfigWatcher>
#include <KSharedConfig>
#include <QObject>

class GlobalConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int volumeStep READ volumeStep NOTIFY volumeStepChanged)

public:
    explicit GlobalConfig(QObject *parent = nullptr);
    ~GlobalConfig() override;

public Q_SLOTS:
    Q_INVOKABLE int volumeStep();

Q_SIGNALS:
    void volumeStepChanged() const;

private:
    KConfigWatcher::Ptr m_configWatcher;
    int m_volumeStep;

private Q_SLOTS:
    void configChanged();
};
