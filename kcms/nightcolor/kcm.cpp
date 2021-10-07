/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>

#include "nightcolordata.h"

namespace ColorCorrect
{
K_PLUGIN_FACTORY_WITH_JSON(KCMNightColorFactory, "kcm_nightcolor.json", registerPlugin<KCMNightColor>(); registerPlugin<NightColorData>();)

KCMNightColor::KCMNightColor(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_data(new NightColorData(this))
{
    qmlRegisterAnonymousType<NightColorSettings>("org.kde.private.kcms.nightcolor", 1);
    qmlRegisterUncreatableMetaObject(ColorCorrect::staticMetaObject, "org.kde.private.kcms.nightcolor", 1, 0, "NightColorMode", "Error: only enums");

    KAboutData *about = new KAboutData(QStringLiteral("kcm_nightcolor"), i18n("Night Color"), QStringLiteral("0.1"), QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Roman Gilg"), QString(), QStringLiteral("subdiff@gmail.com"));
    setAboutData(about);
    setButtons(Apply | Default);
}

NightColorSettings *KCMNightColor::nightColorSettings() const
{
    return m_data->settings();
}

}

#include "kcm.moc"
