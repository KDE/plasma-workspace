/*
    SPDX-FileCopyrightText: 2022 Tanbir Jishan <tantalising007@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "colorssettings.h"

#include <kdedmodule.h>

class AccentColorService : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.plasmashell.accentColor")
public:
    explicit AccentColorService(QObject *parent, const QList<QVariant> &);

public Q_SLOTS:
    void setAccentColor(unsigned accentColor);

private:
    ColorsSettings *m_settings;
};
