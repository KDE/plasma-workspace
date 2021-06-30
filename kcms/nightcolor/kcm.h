/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KQuickAddons/ConfigModule>

namespace ColorCorrect
{
class KCMNightColor : public KQuickAddons::ConfigModule
{
    Q_OBJECT
public:
    KCMNightColor(QObject *parent, const QVariantList &args);
    ~KCMNightColor() override
    {
    }

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void loadRelay();
    void saveRelay();
    void defaultsRelay();
};

}
