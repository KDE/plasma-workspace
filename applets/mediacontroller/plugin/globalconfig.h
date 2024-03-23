/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>

#include <KConfigWatcher>
#include <KSharedConfig>

class GlobalConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int volumeStep READ volumeStep NOTIFY volumeStepChanged)
    Q_PROPERTY(QString preferredPlayer READ preferredPlayer NOTIFY preferredPlayerChanged)

public:
    explicit GlobalConfig(QObject *parent = nullptr);
    ~GlobalConfig() override;

    int volumeStep() const;
    QString preferredPlayer() const;

Q_SIGNALS:
    void volumeStepChanged() const;
    void preferredPlayerChanged();

private:
    void onConfigChanged();

    KConfigWatcher::Ptr m_configWatcher;
    int m_volumeStep;
    QString m_preferredPlayer;
};
