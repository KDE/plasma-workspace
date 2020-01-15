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
    , m_plasmaConfig(KSharedConfig::openConfig(QStringLiteral("PlasmaUserFeedback")))
{
    Q_UNUSED(args)
    setAboutData(new KAboutData(QStringLiteral("kcm_feedback"),
                                       i18n("User Feedback"),
                                       QStringLiteral("1.0"), i18n("Configure user feedback settings"), KAboutLicense::LGPL));

    connect(this, &Feedback::plasmaFeedbackLevelChanged, this, [this](){
        setNeedsSave(true);
    });
}

Feedback::~Feedback() = default;

bool Feedback::feedbackEnabled() const
{
    KUserFeedback::Provider p;
    return p.isEnabled();
}

void Feedback::load()
{
    //We only operate if the kill switch is off, all KDE components should default to KUserFeedback::Provider::NoTelemetry
    setPlasmaFeedbackLevel(m_plasmaConfig->group("Global").readEntry("FeedbackLevel", int(KUserFeedback::Provider::NoTelemetry)));
    setNeedsSave(false);
}

void Feedback::save()
{
    m_plasmaConfig->group("Global").writeEntry("FeedbackLevel", m_plasmaFeedbackLevel);
    m_plasmaConfig->sync();
}

void Feedback::defaults()
{
    setPlasmaFeedbackLevel(KUserFeedback::Provider::NoTelemetry);
}

void Feedback::setPlasmaFeedbackLevel(int plasmaFeedbackLevel) {
    if (plasmaFeedbackLevel != m_plasmaFeedbackLevel) {
        m_plasmaFeedbackLevel = plasmaFeedbackLevel;
        Q_EMIT plasmaFeedbackLevelChanged(plasmaFeedbackLevel);
    }
    setRepresentsDefaults(plasmaFeedbackLevel == KUserFeedback::Provider::NoTelemetry);
}

#include "feedback.moc"
