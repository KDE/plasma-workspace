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
    Q_PROPERTY(bool inhibited MEMBER inhibited NOTIFY inhibitedChanged)
public:
    KCMNightLight(QObject *parent, const KPluginMetaData &data);
    ~KCMNightLight() override = default;

    NightLightSettings *nightLightSettings() const;
    Q_INVOKABLE bool isIconThemeBreeze();

private:
    NightLightData *const m_data;
    int minDayTemp;
    int maxDayTemp;
    int minNightTemp;
    int maxNightTemp;
    bool inhibited = false;

    void updateProperties(const QVariantMap &properties);

private Q_SLOTS:
    void handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

public Q_SLOTS:
    void save() override;

Q_SIGNALS:
    void inhibitedChanged(bool inhibited);
};

}
