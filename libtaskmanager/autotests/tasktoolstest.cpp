/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KSycoca>

#include <QDir>
#include <QIcon>
#include <QStandardPaths>
#include <QTest>

#include "tasktools.h"

using namespace TaskManager;

class TaskToolsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void shouldFindApp();
    void shouldFindApp_data();
    void shouldFindDefaultApp();
    void shouldCompareLauncherUrls();
    void testWindowUrlFromMetadata();
    void testWindowUrlFromMetadata_data();

private:
    void createIcon();
};

void TaskToolsTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    qputenv("XDG_DATA_DIRS", dataDir.toUtf8());

    // Make sure we start with a clean dir
    QVERIFY(QDir(dataDir).removeRecursively());
    QVERIFY(QDir(dataDir).mkpath(QLatin1String("applications")));
    QVERIFY(QDir(dataDir).mkpath(QLatin1String("kservices5")));

    // Add our applications
    QFile::copy(QFINDTESTDATA("data/applications/org.kde.dolphin.desktop"), dataDir + QLatin1String("/applications/org.kde.dolphin.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/org.kde.konversation.desktop"), dataDir + QLatin1String("/applications/org.kde.konversation.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/im.riot.Riot.desktop"), dataDir + QLatin1String("/applications/im.riot.Riot.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/org.telegram.desktop.desktop"), dataDir + QLatin1String("/applications/org.telegram.desktop.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/com.spotify.Client.desktop"), dataDir + QLatin1String("/applications/com.spotify.Client.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/GammaRay.desktop"), dataDir + QLatin1String("/applications/GammaRay.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/org.kde.gwenview_importer.desktop"),
                dataDir + QLatin1String("/applications/org.kde.gwenview_importer.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/kcm_autostart.desktop"), dataDir + QLatin1String("/applications/kcm_autostart.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/brave-browser.desktop"), dataDir + QLatin1String("/applications/brave-browser.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/brave-efmjfjelnicpmdcmfikempdhlmainjcb-Default.desktop"),
                dataDir + QLatin1String("/applications/brave-efmjfjelnicpmdcmfikempdhlmainjcb-Default.desktop"));

    QFile::copy(QFINDTESTDATA("data/applications/kcm_kdeconnect.desktop"), dataDir + QLatin1String("/kservices5/kcm_kdeconnect.desktop"));

    QFile::remove(KSycoca::absoluteFilePath());
    KSycoca::self()->ensureCacheValid();
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));

    // Verify that our enviromnent is as expected and no outside apps leak in
    QVERIFY(!KService::serviceByDesktopName(QStringLiteral("org.kde.ktrip")));
    QVERIFY(KService::serviceByDesktopName(QStringLiteral("org.kde.dolphin")));
    QVERIFY(KService::serviceByDesktopName(QStringLiteral("org.kde.konversation")));
}

void TaskToolsTest::shouldFindApp_data()
{
    QTest::addColumn<QString>("inputFileName");
    QTest::addColumn<QString>("id");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("genericName");
    QTest::addColumn<QUrl>("url");

    QTest::newRow("Konversation") << QStringLiteral("org.kde.konversation.desktop") << QStringLiteral("org.kde.konversation") << QStringLiteral("Konversation")
                                  << QStringLiteral("IRC Client") << QUrl(QStringLiteral("applications:org.kde.konversation.desktop"));

    QTest::newRow("Dolphin") << QStringLiteral("org.kde.dolphin.desktop") << QStringLiteral("org.kde.dolphin") << QStringLiteral("Dolphin")
                             << QStringLiteral("File Manager") << QUrl(QStringLiteral("applications:org.kde.dolphin.desktop"));
}

void TaskToolsTest::shouldFindApp()
{
    // FIXME Test icon.

    QFETCH(QString, inputFileName);

    const QUrl inputUrl = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("applications/") + inputFileName));

    const AppData &data = appDataFromUrl(inputUrl);

    QFETCH(QString, id);
    QFETCH(QString, name);
    QFETCH(QString, genericName);
    QFETCH(QUrl, url);

    QCOMPARE(data.id, id);
    QCOMPARE(data.name, name);
    QCOMPARE(data.genericName, genericName);
    QCOMPARE(data.url, url);
}

void TaskToolsTest::shouldFindDefaultApp()
{
    // FIXME Test other recognized default app types.

    KConfigGroup config(KSharedConfig::openConfig(), "General");
    config.writePathEntry("BrowserApplication", QLatin1String("konqueror"));

    QVERIFY(defaultApplication(QUrl("wrong://url")).isEmpty());
    QCOMPARE(defaultApplication(QUrl("preferred://browser")), QLatin1String("konqueror"));
}

void TaskToolsTest::shouldCompareLauncherUrls()
{
    QUrl a(QLatin1String("file:///usr/share/applications/org.kde.dolphin.desktop"));
    QUrl b(QLatin1String("file:///usr/share/applications/org.kde.konsole.desktop"));
    QUrl c(QLatin1String("file:///usr/share/applications/org.kde.dolphin.desktop?iconData=foo"));
    QUrl d(QLatin1String("file:///usr/share/applications/org.kde.konsole.desktop?iconData=bar"));

    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(a)));
    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(a), Strict));
    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(a), IgnoreQueryItems));

    QVERIFY(!launcherUrlsMatch(QUrl(a), QUrl(b)));
    QVERIFY(!launcherUrlsMatch(QUrl(a), QUrl(b), Strict));
    QVERIFY(!launcherUrlsMatch(QUrl(a), QUrl(b), IgnoreQueryItems));

    QVERIFY(!launcherUrlsMatch(QUrl(b), QUrl(c), Strict));
    QVERIFY(!launcherUrlsMatch(QUrl(c), QUrl(d), Strict));

    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(c), IgnoreQueryItems));
    QVERIFY(!launcherUrlsMatch(QUrl(c), QUrl(d), IgnoreQueryItems));
}

void TaskToolsTest::testWindowUrlFromMetadata()
{
    QFETCH(QString, appId);
    QFETCH(QString, xWindowsWMClassName);
    QFETCH(QUrl, resultUrl);

    const QUrl actualResult = windowUrlFromMetadata(appId, 0, KSharedConfig::openConfig(QStringLiteral("taskmanagerrulestestrc")), xWindowsWMClassName);

    QCOMPARE(actualResult, resultUrl);
}

void TaskToolsTest::testWindowUrlFromMetadata_data()
{
    QTest::addColumn<QString>("appId");
    QTest::addColumn<QString>("xWindowsWMClassName");
    QTest::addColumn<QUrl>("resultUrl");

    QTest::addRow("Dolphin") << QStringLiteral("org.kde.dolphin") << QString() << QUrl(QStringLiteral("applications:org.kde.dolphin.desktop"));
    QTest::addRow("Element (Flatpak)") << QStringLiteral("Element") << QStringLiteral("element") << QUrl(QStringLiteral("applications:im.riot.Riot.desktop"));
    QTest::addRow("Telegram (Flatpak)") << QStringLiteral("TelegramDesktop") << QStringLiteral("telegram-desktop")
                                        << QUrl(QStringLiteral("applications:org.telegram.desktop.desktop"));
    QTest::addRow("Spotify (Flatpak)") << QStringLiteral("Spotify") << QStringLiteral("spotify")
                                       << QUrl(QStringLiteral("applications:com.spotify.Client.desktop"));
    QTest::addRow("GammaRay") << QStringLiteral("GammaRay") << QStringLiteral("gammary-client") << QUrl(QStringLiteral("applications:GammaRay.desktop"));
    QTest::addRow("Gwenview Importer") << QStringLiteral("org.kde.gwenview_importer") << QStringLiteral("gwenview_importer")
                                       << QUrl(QStringLiteral("applications:org.kde.gwenview_importer.desktop"));
    QTest::addRow("kcm_autostart") << QStringLiteral("kcm_autostart") << QString() << QUrl(QStringLiteral("applications:kcm_autostart.desktop"));
    QTest::addRow("brave") << QStringLiteral("Brave-browser") << QStringLiteral("brave-browser") << QUrl("applications:brave-browser.desktop");
    QTest::addRow("brave_webapp") << QStringLiteral("Brave-browser") << QStringLiteral("crx_efmjfjelnicpmdcmfikempdhlmainjcb")
                                  << QUrl("applications:brave-efmjfjelnicpmdcmfikempdhlmainjcb-Default.desktop");

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QTest::addRow("kcm_kdeconnect") << dataDir + QLatin1String("/kservices5/kcm_kdeconnect") << QString()
                                    << QUrl::fromLocalFile(dataDir + QLatin1String("/kservices5/kcm_kdeconnect.desktop"));

    // TODO test mapping rules
}

QTEST_MAIN(TaskToolsTest)

#include "tasktoolstest.moc"
