/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KQuickAddons/ManagedConfigModule>

#include "nightcolorsettings.h"

class NightColorData;

namespace ColorCorrect
{
class KCMNightColor : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(NightColorSettings *nightColorSettings READ nightColorSettings CONSTANT)
public:
    KCMNightColor(QObject *parent, const QVariantList &args);
    ~KCMNightColor() override = default;

    NightColorSettings *nightColorSettings() const;

private:
    NightColorData *m_data;
};

}
