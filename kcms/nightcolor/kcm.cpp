/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"

#include <QIcon>
#include <QStandardPaths>
#include <QDBusConnection>
#include <QDBusMessage>

#include <KLocalizedString>
#include <KPluginFactory>

#include "nightcolordata.h"

namespace ColorCorrect
{
K_PLUGIN_FACTORY_WITH_JSON(KCMNightColorFactory, "kcm_nightcolor.json", registerPlugin<KCMNightColor>(); registerPlugin<NightColorData>();)

KCMNightColor::KCMNightColor(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_data(new NightColorData(this))
{
    qmlRegisterAnonymousType<NightColorSettings>("org.kde.private.kcms.nightcolor", 1);
    qmlRegisterUncreatableMetaObject(ColorCorrect::staticMetaObject, "org.kde.private.kcms.nightcolor", 1, 0, "NightColorMode", "Error: only enums");

    worldMapFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("plasma/nightcolor/worldmap.png"), QStandardPaths::LocateFile);

    minDayTemp = nightColorSettings()->findItem("DayTemperature")->minValue().toInt();
    maxDayTemp = nightColorSettings()->findItem("DayTemperature")->maxValue().toInt();
    minNightTemp = nightColorSettings()->findItem("NightTemperature")->minValue().toInt();
    maxNightTemp = nightColorSettings()->findItem("NightTemperature")->maxValue().toInt();

    setButtons(Apply | Default);
}

NightColorSettings *KCMNightColor::nightColorSettings() const
{
    return m_data->settings();
}

// FIXME: This was added to work around the nonstandardness of the Breeze zoom icons
// remove once https://bugs.kde.org/show_bug.cgi?id=435671 is fixed
bool KCMNightColor::isIconThemeBreeze()
{
    return QIcon::themeName().contains(QStringLiteral("breeze"));
}

void KCMNightColor::save()
{
    KQuickAddons::ManagedConfigModule::save();

    // load/unload colorcorrectlocationupdater based on whether user set it to automatic location
    QDBusConnection dbus = QDBusConnection::sessionBus();

    bool enableUpdater = (nightColorSettings()->mode() == NightColorMode::Automatic);

    QDBusMessage loadMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kded5"),
                                                          QStringLiteral("/kded"),
                                                          QStringLiteral("org.kde.kded5"),
                                                          (enableUpdater ? QStringLiteral("loadModule") : QStringLiteral("unloadModule")));
    loadMsg.setArguments({QVariant(QStringLiteral("colorcorrectlocationupdater"))});
    dbus.call(loadMsg, QDBus::NoBlock);
}
}
#include "kcm.moc"
