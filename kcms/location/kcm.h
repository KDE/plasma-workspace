/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickManagedConfigModule>

class LocationData;
class LocationSettings;

class LocationKcm : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(LocationSettings *settings READ settings CONSTANT)

public:
    explicit LocationKcm(QObject *parent, const KPluginMetaData &data);

    LocationSettings *settings() const;

private:
    LocationData *m_data;
};
