/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

#include <QIcon>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KPluginFactory>

#include "nightlightdata.h"

namespace ColorCorrect
{
K_PLUGIN_FACTORY_WITH_JSON(KCMNightLightFactory, "kcm_nightlight.json", registerPlugin<KCMNightLight>(); registerPlugin<NightLightData>();)

static const QString s_service = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_path = QStringLiteral("/org/kde/KWin/NightLight");
static const QString s_interface = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");

KCMNightLight::KCMNightLight(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new NightLightData(this))
{
    qmlRegisterAnonymousType<NightLightSettings>("org.kde.private.kcms.nightlight", 1);
    qmlRegisterUncreatableMetaObject(ColorCorrect::staticMetaObject, "org.kde.private.kcms.nightlight", 1, 0, "NightLightMode", "Error: only enums");

    minDayTemp = nightLightSettings()->findItem("DayTemperature")->minValue().toInt();
    maxDayTemp = nightLightSettings()->findItem("DayTemperature")->maxValue().toInt();
    minNightTemp = nightLightSettings()->findItem("NightTemperature")->minValue().toInt();
    maxNightTemp = nightLightSettings()->findItem("NightTemperature")->maxValue().toInt();

    QDBusConnection bus = QDBusConnection::sessionBus();

    QDBusMessage message = QDBusMessage::createMethodCall(s_service, s_path, s_propertiesInterface, QStringLiteral("GetAll"));
    message.setArguments({s_interface});
    QDBusPendingReply<QVariantMap> properties = bus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(properties, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        self->deleteLater();
        const QDBusPendingReply<QVariantMap> properties = *self;
        if (properties.isError()) {
            return;
        }
        updateProperties(properties.value());
    });

    bus.connect(s_service,
                s_path,
                s_propertiesInterface,
                QStringLiteral("PropertiesChanged"),
                this,
                SLOT(handlePropertiesChanged(QString, QVariantMap, QStringList)));

    setButtons(Apply | Default);
}

NightLightSettings *KCMNightLight::nightLightSettings() const
{
    return m_data->settings();
}

// FIXME: This was added to work around the nonstandardness of the Breeze zoom icons
// remove once https://bugs.kde.org/show_bug.cgi?id=435671 is fixed
bool KCMNightLight::isIconThemeBreeze()
{
    return QIcon::themeName().contains(QStringLiteral("breeze"));
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

void KCMNightLight::handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    updateProperties(changedProperties);
}

void KCMNightLight::updateProperties(const QVariantMap &properties)
{
    const QVariant newInhibited = properties.value(QStringLiteral("inhibited"));
    if (newInhibited.isValid()) {
        inhibited = newInhibited.toBool();
        Q_EMIT inhibitedChanged(inhibited);
    }
}
}
#include "kcm.moc"
