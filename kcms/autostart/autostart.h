/*
    SPDX-FileCopyrightText: 2006-2007 Stephen Leaf <smileaf@gmail.com>
    SPDX-FileCopyrightText: 2008 Montel Laurent <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickAddons/ConfigModule>

#include "autostartmodel.h"

class Autostart : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(AutostartModel *model READ model CONSTANT)

public:
    explicit Autostart(QObject *parent, const KPluginMetaData &data, const QVariantList &);
    ~Autostart() override;

    void load() override;
    void save() override;
    void defaults() override;

    AutostartModel *model() const;

private:
    AutostartModel *m_model;
};
