/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"
#include "darklightschedulepreview.h"
#include "darklightschedulevalidator.h"
#include "dashedbackground.h"
#include "nighttimedata.h"
#include "nighttimesettings.h"
#include "sunpathchart.h"

#include <KLocalizedString>
#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(KCMNightTimeFactory, "kcm_nighttime.json", registerPlugin<KCMNightTime>(); registerPlugin<NightTimeData>();)

KCMNightTime::KCMNightTime(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new NightTimeData(this))
{
    const auto uri = "org.kde.private.kcms.nighttime";
    qmlRegisterType<DarkLightSchedulePreview>(uri, 1, 0, "DarkLightSchedulePreview");
    qmlRegisterType<SunPathChart>(uri, 1, 0, "SunPathChart");
    qmlRegisterType<DashedBackground>(uri, 1, 0, "DashedBackground");
    qmlRegisterUncreatableType<NightTimeSettings>(uri, 1, 0, "NightTimeSettings", QStringLiteral("Settings"));
    qmlRegisterSingletonType<DarkLightScheduleValidator>(uri, 1, 0, "DarkLightScheduleValidator", [](QQmlEngine *, QJSEngine *) {
        return new DarkLightScheduleValidator();
    });

    setButtons(Apply | Default);
}

NightTimeSettings *KCMNightTime::settings() const
{
    return m_data->settings();
}

#include "kcm.moc"
#include "moc_kcm.cpp"
