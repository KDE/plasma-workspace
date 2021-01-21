/*
 *   Copyright 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   Copyright 2020 David Edmundson <davidedmundson@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "userfeedback.h"

#include <KUserFeedback/ApplicationVersionSource>
#include <KUserFeedback/CompilerInfoSource>
#include <KUserFeedback/OpenGLInfoSource>
#include <KUserFeedback/PlatformInfoSource>
#include <KUserFeedback/Provider>
#include <KUserFeedback/QPAInfoSource>
#include <KUserFeedback/QtVersionSource>
#include <KUserFeedback/ScreenInfoSource>
#include <KUserFeedback/UsageTimeSource>

#include <KLocalizedString>

#include "shellcorona.h"

class PanelCountSource : public KUserFeedback::AbstractDataSource
{
public:
    /*! Create a new start count data source. */
    PanelCountSource(ShellCorona *corona)
        : AbstractDataSource(QStringLiteral("panelCount"), KUserFeedback::Provider::DetailedSystemInformation)
        , corona(corona)
    {
    }

    QString name() const override
    {
        return i18n("Panel Count");
    }
    QString description() const override
    {
        return i18n("Counts the panels");
    }

    QVariant data() override
    {
        return QVariantMap{{QStringLiteral("panelCount"), corona->panelCount()}};
    }

private:
    ShellCorona *const corona;
};

UserFeedback::UserFeedback(ShellCorona *corona, QObject *parent)
    : QObject(parent)
    , m_provider(new KUserFeedback::Provider(this))
{
    m_provider->setProductIdentifier(QStringLiteral("org.kde.plasmashell"));
    m_provider->setFeedbackServer(QUrl(QStringLiteral("https://telemetry.kde.org/")));
    m_provider->setSubmissionInterval(7);
    m_provider->setApplicationStartsUntilEncouragement(5);
    m_provider->setEncouragementDelay(30);
    m_provider->addDataSource(new KUserFeedback::ApplicationVersionSource);
    m_provider->addDataSource(new KUserFeedback::CompilerInfoSource);
    m_provider->addDataSource(new KUserFeedback::PlatformInfoSource);
    m_provider->addDataSource(new KUserFeedback::QtVersionSource);
    m_provider->addDataSource(new KUserFeedback::UsageTimeSource);
    m_provider->addDataSource(new KUserFeedback::OpenGLInfoSource);
    m_provider->addDataSource(new KUserFeedback::ScreenInfoSource);
    m_provider->addDataSource(new KUserFeedback::QPAInfoSource);
    m_provider->addDataSource(new PanelCountSource(corona));

    auto plasmaConfig = KSharedConfig::openConfig(QStringLiteral("PlasmaUserFeedback"));
    m_provider->setTelemetryMode(
        KUserFeedback::Provider::TelemetryMode(plasmaConfig->group("Global").readEntry("FeedbackLevel", int(KUserFeedback::Provider::NoTelemetry))));
}

QString UserFeedback::describeDataSources() const
{
    return m_provider->describeDataSources();
}
