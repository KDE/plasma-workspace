/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KQuickManagedConfigModule>

#include "nightlightsettings.h"

class NightLightData;

namespace ColorCorrect
{
class KCMNightLight : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(NightLightSettings *nightLightSettings READ nightLightSettings CONSTANT)
    Q_PROPERTY(int minDayTemp MEMBER minDayTemp CONSTANT)
    Q_PROPERTY(int maxDayTemp MEMBER maxDayTemp CONSTANT)
    Q_PROPERTY(int minNightTemp MEMBER minNightTemp CONSTANT)
    Q_PROPERTY(int maxNightTemp MEMBER maxNightTemp CONSTANT)
    Q_PROPERTY(bool configurationInvalid READ isConfigurationInvalid CONSTANT)
public:
    KCMNightLight(QObject *parent, const KPluginMetaData &data);
    ~KCMNightLight() override = default;

    NightLightSettings *nightLightSettings() const;
    Q_INVOKABLE bool isConfigurationInvalid();

private:
    NightLightData *const m_data;
    int minDayTemp;
    int maxDayTemp;
    int minNightTemp;
    int maxNightTemp;

public Q_SLOTS:
    void save() override;
};

}
