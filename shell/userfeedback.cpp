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

#include <KSharedConfig>
#include <KConfigGroup>

#include <KLocalizedString>

#include "shellcorona.h"

// Contains a collection of all sources for Plasma related things
// When updating code here be sure to update the schema on the server

class PanelCountSource : public KUserFeedback::AbstractDataSource
{
public:
    /*! Create a new start count data source. */
    PanelCountSource(ShellCorona* corona)
        : AbstractDataSource(QStringLiteral("panelCount"), KUserFeedback::Provider::DetailedSystemInformation)
        , corona(corona)
    {}

    QString name() const override { return i18n("Panel Count"); }
    QString description() const override { return i18n("Counts the panels"); }

    QVariant data() override { return QVariantMap{ { QStringLiteral("panelCount"), corona->panelCount() } } ; }

private:
    ShellCorona* const corona;
};

class ThemeSettingsSource: public KUserFeedback::AbstractDataSource
{
public:
    ThemeSettingsSource()
        : AbstractDataSource(QStringLiteral("themeSettings"), KUserFeedback::Provider::DetailedSystemInformation)
    {}

    QString name() const override { return i18n("Theme Settings"); }
    QString description() const override { return i18n("Theme settings used on the Plasma desktop"); }
    QVariant data() override {
        QVariantMap data;
        KConfigGroup kdeSettings = KSharedConfig::openConfig()->group("KDE");
        data["singleClick"] = kdeSettings.readEntry("SingleClick", true);
        data["lookAndFeelPackage"] = kdeSettings.readEntry("LookAndFeelPackage", QString());
        data["widgetStyle"] = kdeSettings.readEntry("widgetStyle", QString());
        data["iconTheme"] = KSharedConfig::openConfig()->group("Icons").readEntry("Theme", QString());
        data["colorScheme"] = KSharedConfig::openConfig()->group("General").readEntry("ColorScheme", QString());
        return data;
    }
};

class AppletListSource: public KUserFeedback::AbstractDataSource
{
public:
    AppletListSource(ShellCorona *corona)
        : AbstractDataSource(QStringLiteral("applets"), KUserFeedback::Provider::DetailedSystemInformation)
        , corona(corona)
    {}
    QString name() const override { return i18n("Applets"); }
    QString description() const override { return i18n("List of running applets"); }

    QVariant data() override {
        QStringList applets;
        for(auto c: corona->containments()) {
            for (auto applet: c->applets()) {
                QString appletName = applet->pluginMetaData().pluginId();
                if (!appletName.startsWith("org.kde.")) {
                    // if it's not from us, it's probably in the form "net.davidedmundson.superawesomesecretapplet"
                    // at which point including it could leak a probable identifier
                   // by taking a hash, we can hide that but still see how popular net.some.random.store.thing is if we know to search for that
                    appletName = QCryptographicHash::hash(appletName.toLatin1(), QCryptographicHash::Md5);
                }
                applets << appletName;
            }
        }
        return applets;
    }
private:
    ShellCorona* const corona;
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
    m_provider->addDataSource(new ThemeSettingsSource);
    m_provider->addDataSource(new AppletListSource(corona));

    auto plasmaConfig = KSharedConfig::openConfig(QStringLiteral("PlasmaUserFeedback"));
    m_provider->setTelemetryMode(KUserFeedback::Provider::TelemetryMode(plasmaConfig->group("Global").readEntry("FeedbackLevel", int(KUserFeedback::Provider::NoTelemetry))));
}

QString UserFeedback::describeDataSources() const
{
    return m_provider->describeDataSources();
}
