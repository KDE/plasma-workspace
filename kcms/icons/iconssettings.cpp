/*
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QApplication>
#include <QDebug>
#include <QObject>

#include <KBuildSycocaProgressDialog>
#include <KIconTheme>
#include <KSharedDataCache>

#include "iconssettings.h"

IconsSettings::IconsSettings(QObject *parent)
    : IconsSettingsBase(parent)
    , m_themeDirty(false)
{
    connect(this, &IconsSettings::configChanged, this, &IconsSettings::updateIconTheme);
    connect(this, &IconsSettings::ThemeChanged, this, &IconsSettings::updateThemeDirty);
}

IconsSettings::~IconsSettings()
{
}

void IconsSettings::updateThemeDirty()
{
    m_themeDirty = theme() != KIconTheme::current();
}

void IconsSettings::updateIconTheme()
{
    if (m_themeDirty) {
        KIconTheme::reconfigure();

        KSharedDataCache::deleteCache(QStringLiteral("icon-cache"));

        for (int i = 0; i < KIconLoader::LastGroup; i++) {
            KIconLoader::emitChange(KIconLoader::Group(i));
        }

        KBuildSycocaProgressDialog::rebuildKSycoca(QApplication::activeWindow());
    }
}
