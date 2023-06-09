/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KQuickManagedConfigModule>

#include "nightcolorsettings.h"

class NightColorData;

namespace ColorCorrect
{
class KCMNightColor : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(NightColorSettings *nightColorSettings READ nightColorSettings CONSTANT)
    Q_PROPERTY(int minDayTemp MEMBER minDayTemp CONSTANT)
    Q_PROPERTY(int maxDayTemp MEMBER maxDayTemp CONSTANT)
    Q_PROPERTY(int minNightTemp MEMBER minNightTemp CONSTANT)
    Q_PROPERTY(int maxNightTemp MEMBER maxNightTemp CONSTANT)
public:
    KCMNightColor(QObject *parent, const KPluginMetaData &data);
    ~KCMNightColor() override = default;

    NightColorSettings *nightColorSettings() const;
    Q_INVOKABLE bool isIconThemeBreeze();

private:
    NightColorData *const m_data;
    int minDayTemp;
    int maxDayTemp;
    int minNightTemp;
    int maxNightTemp;

public Q_SLOTS:
    void save() override;
};

}
