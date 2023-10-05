/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "colorblindnesscorrection_config.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>

#include <KPluginFactory>

#include "colorblindnesscorrection_settings.h"
#include "colorblindnesscorrection_settingsdata.h"

K_PLUGIN_CLASS_WITH_JSON(KWin::ColorBlindnessCorrectionEffectConfig, "kwin_colorblindnesscorrection_config.json")

namespace KWin
{

ColorBlindnessCorrectionEffectConfig::ColorBlindnessCorrectionEffectConfig(QObject *parent, const KPluginMetaData &metaData)
    : KQuickManagedConfigModule(parent, metaData)
    , m_data(new ColorBlindnessCorrectionSettingsData(this))
{
    qmlRegisterUncreatableType<ColorBlindnessCorrectionSettings>("org.kde.plasma.kwin.colorblindnesscorrectioneffect.kcm",
                                                                 1,
                                                                 0,
                                                                 "ColorBlindnessCorrectionSettings",
                                                                 QStringLiteral("Only for enums"));

    setButtons(Apply | Default);
}

ColorBlindnessCorrectionEffectConfig::~ColorBlindnessCorrectionEffectConfig()
{
}

ColorBlindnessCorrectionSettings *ColorBlindnessCorrectionEffectConfig::settings() const
{
    return m_data->settings();
}

void ColorBlindnessCorrectionEffectConfig::save()
{
    KQuickManagedConfigModule::save();

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                              QStringLiteral("/Effects"),
                                              QStringLiteral("org.kde.kwin.Effects"),
                                              QStringLiteral("reconfigureEffect"));
    msg.setArguments({QStringLiteral("kwin_effect_colorblindnesscorrection")});
    QDBusConnection::sessionBus().asyncCall(msg);
}

} // namespace

#include "colorblindnesscorrection_config.moc"
#include "moc_colorblindnesscorrection_config.cpp"
