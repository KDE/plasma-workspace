/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QIcon>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KPluginFactory>

#include "nightlightdata.h"

namespace ColorCorrect
{
K_PLUGIN_FACTORY_WITH_JSON(KCMNightLightFactory, "kcm_nightlight.json", registerPlugin<KCMNightLight>(); registerPlugin<NightLightData>();)

KCMNightLight::KCMNightLight(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new NightLightData(this))
{
    const auto uri = "org.kde.private.kcms.nightlight";
    qmlRegisterAnonymousType<NightLightSettings>(uri, 1);
    qmlRegisterUncreatableMetaObject(ColorCorrect::staticMetaObject, uri, 1, 0, "NightLightMode", "Error: only enums");

    minDayTemp = nightLightSettings()->findItem("DayTemperature")->minValue().toInt();
    maxDayTemp = nightLightSettings()->findItem("DayTemperature")->maxValue().toInt();
    minNightTemp = nightLightSettings()->findItem("NightTemperature")->minValue().toInt();
    maxNightTemp = nightLightSettings()->findItem("NightTemperature")->maxValue().toInt();

    setButtons(Apply | Default);
}

NightLightSettings *KCMNightLight::nightLightSettings() const
{
    return m_data->settings();
}

void KCMNightLight::save()
{
    KQuickManagedConfigModule::save();

    // load/unload colorcorrectlocationupdater based on whether user set it to automatic location
    QDBusConnection dbus = QDBusConnection::sessionBus();

    bool enableUpdater = (nightLightSettings()->mode() == NightLightMode::Automatic);

    QDBusMessage loadMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kded6"),
                                                          QStringLiteral("/kded"),
                                                          QStringLiteral("org.kde.kded6"),
                                                          (enableUpdater ? QStringLiteral("loadModule") : QStringLiteral("unloadModule")));
    loadMsg.setArguments({QVariant(QStringLiteral("colorcorrectlocationupdater"))});
    dbus.call(loadMsg, QDBus::NoBlock);
}
}
#include "kcm.moc"
