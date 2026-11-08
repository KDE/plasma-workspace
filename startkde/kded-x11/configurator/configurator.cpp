/*
    SPDX-FileCopyrightText: 2026 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configurator.h"
#include "../../../kcms/kcms-common_p.h"
#include "xsystemsettings.h"

#include <KPluginFactory>

#include <QDBusConnection>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(Configurator, "configurator.json")

Configurator::Configurator(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_kwinrcWatcher(KConfigWatcher::create(KSharedConfig::openConfig(u"kwinrc"_s)))
{
    auto bus = QDBusConnection::sessionBus();
    bus.connect(QString(), u"/KGlobalSettings"_s, u"org.kde.KGlobalSettings"_s, u"notifyChange"_s, this, SLOT(onNotifyChange(int, int)));
    bus.connect(QString(), u"/KDEPlatformTheme"_s, u"org.kde.KDEPlatformTheme"_s, u"refreshFonts"_s, this, SLOT(onRefreshFonts()));

    connect(m_kwinrcWatcher.get(), &KConfigWatcher::configChanged, this, [](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.name() == u"Xwayland"_s && names.contains("Scale"_ba)) {
            XSystemSettings::sync();
        }
    });
}

void Configurator::onNotifyChange(int type, int arg)
{
    Q_UNUSED(arg)

    if (type == GlobalChangeType::CursorChanged) {
        XSystemSettings::sync();
    }
}

void Configurator::onRefreshFonts()
{
    XSystemSettings::sync();
}

#include "configurator.moc"
#include "moc_configurator.cpp"
