/*
    SPDX-FileCopyrightText: 2022 Natalie Clarius <natalie_clarius@yahoo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "colorsmodel.h"
#include "colorssettings.h"

#include <kdedmodule.h>

class ColorSchemeService : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.plasmashell.colorScheme")
public:
    explicit ColorSchemeService(QObject *parent, const QList<QVariant> &);

public Q_SLOTS:
    void setColorScheme(QString colorScheme);
    QStringList installedColorSchemes();

private:
    ColorsSettings *m_settings;
    ColorsModel *m_model;
};
