/*
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "iconssettingsbase.h"

class IconsSettings : public IconsSettingsBase
{
    Q_OBJECT
public:
    IconsSettings(QObject *parent = nullptr);
    ~IconsSettings() override;
public Q_SLOTS:
    void updateIconTheme();
    void updateThemeDirty();

private:
    bool m_themeDirty;
};
