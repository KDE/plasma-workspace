/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    m_provider->setTelemetryMode(KUserFeedback::Provider::TelemetryMode(
        plasmaConfig->group(QStringLiteral("Global")).readEntry("FeedbackLevel", int(KUserFeedback::Provider::NoTelemetry))));
}

QString UserFeedback::describeDataSources() const
{
    return m_provider->describeDataSources();
}
