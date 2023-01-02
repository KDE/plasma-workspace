/*
    localegeneratorglibc.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "localegeneratorglibc.h"
#include "kcm_regionandlang_debug.h"

LocaleGeneratorGlibc::LocaleGeneratorGlibc(QObject *parent)
    : LocaleGeneratorBase(parent)
    , m_interface(new LocaleGenHelper(QStringLiteral("org.kde.localegenhelper"), QStringLiteral("/LocaleGenHelper"), QDBusConnection::systemBus(), this))
{
    qCDebug(KCM_REGIONANDLANG) << "connect: " << m_interface->isValid();
    connect(m_interface, &LocaleGenHelper::success, this, &LocaleGeneratorGlibc::needsFont);
    connect(m_interface, &LocaleGenHelper::error, this, &LocaleGeneratorGlibc::userHasToGenerateManually);
}

void LocaleGeneratorGlibc::localesGenerate(const QStringList &list)
{
    qCDebug(KCM_REGIONANDLANG) << "enable locales: " << list;
    if (!QFile::exists(QStringLiteral("/etc/locale.gen"))) {
        // When locale.gen is not present we assume that to mean that no generation is necessary, meaning we are done.
        // e.g. fedora, centos and derivates
        Q_EMIT needsFont();
        return;
    }
    qCDebug(KCM_REGIONANDLANG) << "send polkit request";
    auto reply = m_interface->enableLocales(list);
    if (reply.isError()) {
        Q_EMIT userHasToGenerateManually(defaultManuallyGenerateMessage());
    }

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        if (watcher->isError()) {
            Q_EMIT userHasToGenerateManually(defaultManuallyGenerateMessage());
        }
        watcher->deleteLater();
    });
}
