/*
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PLASMA_DESKTOP_ICONSSETTINGS_H
#define PLASMA_DESKTOP_ICONSSETTINGS_H

#include "iconssettingsbase.h"

class IconsSettings : public IconsSettingsBase
{
    Q_OBJECT
public:
    IconsSettings(QObject *parent = nullptr);
    ~IconsSettings() override;
public slots:
    void updateIconTheme();
    void updateThemeDirty();

private:
    bool m_themeDirty;
};

#endif // PLASMA_DESKTOP_ICONSSETTINGS_H
