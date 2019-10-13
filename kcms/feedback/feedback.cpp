/*
 * Copyright (C) 2019 David Edmundson <davidedmundson@kde.org>
 * Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "feedback.h"

#include <KSharedConfig>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KAboutData>
#include <KLocalizedString>

#include <KUserFeedback/Provider>

K_PLUGIN_CLASS_WITH_JSON(Feedback, "kcm_feedback.json");

Feedback::Feedback(QObject *parent, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent)
    //UserFeedback.conf is used by KUserFeedback which uses QSettings and won't go through globals
    , m_globalConfig(KSharedConfig::openConfig(QStringLiteral("KDE/UserFeedback.conf"), KConfig::NoGlobals))
    , m_plasmaConfig(KSharedConfig::openConfig(QStringLiteral("PlasmaUserFeedback")))
{
    Q_UNUSED(args)
    setAboutData(new KAboutData(QStringLiteral("kcm_feedback"),
                                       i18n("User Feedback"),
                                       QStringLiteral("1.0"), i18n("Configure user feedback settings"), KAboutLicense::LGPL));

    connect(this, &Feedback::feedbackEnabledChanged, this, [this](){
        setNeedsSave(true);
    });
    connect(this, &Feedback::plasmaFeedbackLevelChanged, this, [this](){
        setNeedsSave(true);
    });
}

Feedback::~Feedback() = default;

void Feedback::load()
{
    setFeedbackEnabled(m_globalConfig->group("UserFeedback").readEntry("Enabled", false));

    // See 412913. global killswitch is off by default, therefore globally we're enabled by default :/
    // default to Plama off by default until that situation is resolved to be double sure no user accidentally sends anything
    setPlasmaFeedbackLevel(m_plasmaConfig->group("Global").readEntry("FeedbackLevel", int(KUserFeedback::Provider::NoTelemetry)));
    setNeedsSave(false);
}

void Feedback::save()
{
    m_globalConfig->group("UserFeedback").writeEntry("Enabled", m_feedbackEnabled);
    m_globalConfig->sync();

    m_plasmaConfig->group("Global").writeEntry("FeedbackLevel", m_plasmaFeedbackLevel);
    m_plasmaConfig->sync();
}

void Feedback::defaults()
{
    setFeedbackEnabled(false);
    setPlasmaFeedbackLevel(KUserFeedback::Provider::BasicUsageStatistics);
}

#include "feedback.moc"
