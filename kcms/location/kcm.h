/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickManagedConfigModule>

class GeoClueSettings;

class LocationKcm : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(GeoClueSettings *settings MEMBER m_settings CONSTANT)

public:
    explicit LocationKcm(QObject *parent, const KPluginMetaData &data);

protected:
    void load() override;
    void save() override;
    void defaults() override;

    bool isSaveNeeded() const override;
    bool isDefaults() const override;

private:
    GeoClueSettings *m_settings;
};
