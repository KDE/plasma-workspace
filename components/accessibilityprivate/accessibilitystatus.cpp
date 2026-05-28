/*
    SPDX-FileCopyrightText: 2026 Martin Riethmayer <ripper@freakmail.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "accessibilitystatus.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <kconfigwatcher.h>

using namespace Qt::StringLiterals;

AccessibilityStatus::AccessibilityStatus(QObject *parent)
    : QObject(parent)
{
    m_configWatcher = KConfigWatcher::create(KSharedConfig::openConfig(u"kaccessrc"_s));
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, &AccessibilityStatus::configChanged);
    m_slowKeysEnabled = checkSlowKeyStatus(m_configWatcher->config()->group(QLatin1String("Keyboard")));
}

bool AccessibilityStatus::slowKeysEnabled() const
{
    return m_slowKeysEnabled;
}

void AccessibilityStatus::configChanged(const KConfigGroup &group)
{
    if (group.name() == u"Keyboard") {
        const bool slowKeysEnabled = checkSlowKeyStatus(group);
        if (slowKeysEnabled != m_slowKeysEnabled) {
            m_slowKeysEnabled = slowKeysEnabled;
            Q_EMIT slowKeysEnabledChanged();
        }
    }
}

bool AccessibilityStatus::checkSlowKeyStatus(const KConfigGroup &group) const
{
    return group.readEntry<bool>("SlowKeys", false);
}

#include "moc_accessibilitystatus.cpp"

#include "accessibilitystatus.moc"
