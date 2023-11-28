/*
 * SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-only OR GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "soundthemeconfig.h"

using namespace Qt::StringLiterals;

const QString DEFAULT_SOUND_THEME = u"ocean"_s;

SoundThemeConfig::SoundThemeConfig(QObject *parent)
    : QObject(parent)
    , m_soundTheme(DEFAULT_SOUND_THEME)
{
    m_soundThemeWatcher = KConfigWatcher::create(KSharedConfig::openConfig(u"kdeglobals"_s));
    connect(m_soundThemeWatcher.get(), &KConfigWatcher::configChanged, this, &SoundThemeConfig::kdeglobalsChanged);

    const KConfigGroup soundGroup = m_soundThemeWatcher->config()->group(u"Sounds"_s);
    m_soundTheme = soundGroup.readEntry("Theme", DEFAULT_SOUND_THEME);
}

QString SoundThemeConfig::soundTheme() const
{
    return m_soundTheme;
}

void SoundThemeConfig::kdeglobalsChanged(const KConfigGroup &group, const QByteArrayList &names)
{
    if (group.name() != QLatin1String("Sounds") || !names.contains(QByteArrayLiteral("Theme"))) {
        return;
    }

    m_soundTheme = group.readEntry("Theme", DEFAULT_SOUND_THEME);
    Q_EMIT soundThemeChanged(m_soundTheme);
}

#include "moc_soundthemeconfig.cpp"
