/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>

namespace ColorCorrect
{
K_PLUGIN_CLASS_WITH_JSON(KCMNightColor, "kcm_nightcolor.json")

KCMNightColor::KCMNightColor(QObject *parent, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, args)
{
    KAboutData *about = new KAboutData(QStringLiteral("kcm_nightcolor"), i18n("Night Color"), QStringLiteral("0.1"), QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Roman Gilg"), QString(), QStringLiteral("subdiff@gmail.com"));
    setAboutData(about);
    setButtons(Apply | Default);
}

void KCMNightColor::load()
{
    emit loadRelay();
    setNeedsSave(false);
}

void KCMNightColor::defaults()
{
    emit defaultsRelay();
}

void KCMNightColor::save()
{
    emit saveRelay();
}

}

#include "kcm.moc"
