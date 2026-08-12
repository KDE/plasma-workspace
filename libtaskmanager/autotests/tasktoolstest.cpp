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

using namespace Qt::StringLiterals;
using namespace TaskManager;

class TaskToolsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void shouldFindApp();
    void shouldFindApp_data();
    void testApplicationsUrl();
    void shouldCompareLauncherUrls();
    void testServiceFromMetadata();
    void testServiceFromMetadata_data();
    void testServiceFromCmdLine();
    void testServiceFromCmdLine_data();
    void testServiceForUrl();
    void testServiceForUrl_data();

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
    QVERIFY(QDir(dataDir).mkpath(QLatin1String("kservices6")));

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
    QFile::copy(QFINDTESTDATA("data/applications/marisa..desktop"), dataDir + QLatin1String("/applications/marisa..desktop"));

    QFile::copy(QFINDTESTDATA("data/applications/kcm_kdeconnect.desktop"), dataDir + QLatin1String("/kservices6/kcm_kdeconnect.desktop"));

    QFile::remove(KSycoca::absoluteFilePath());
    KSycoca::self()->ensureCacheValid();
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));

    // Verify that our environment is as expected and no outside apps leak in
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

    const QUrl inputUrl = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString(u"applications/" + inputFileName)));

    const AppData &data = appDataFromUrl(inputUrl);

    QFETCH(QString, id);
    QFETCH(QString, name);
    QFETCH(QString, genericName);
    QFETCH(QUrl, url);

    QCOMPARE(data.service->desktopEntryName(), id);
    QCOMPARE(data.name, name);
    QCOMPARE(data.genericName, genericName);
    QCOMPARE(data.url, url);
}

void TaskToolsTest::testApplicationsUrl()
{
    const QUrl url(u"applications:org.kde.konversation.desktop"_s);

    const AppData &data = appDataFromUrl(url);

    QCOMPARE(data.service->desktopEntryName(), u"org.kde.konversation"_s);
    QCOMPARE(data.name, u"Konversation"_s);
    QCOMPARE(data.genericName, u"IRC Client"_s);
    QCOMPARE(data.url, url);
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

void TaskToolsTest::testServiceFromMetadata()
{
    QFETCH(QString, appId);
    QFETCH(QString, xWindowsWMClassName);
    QFETCH(QString, resultStorageId);

    const auto service = serviceFromMetadata(appId, 0, xWindowsWMClassName);

    QCOMPARE(service->storageId(), resultStorageId);
}

void TaskToolsTest::testServiceFromMetadata_data()
{
    QTest::addColumn<QString>("appId");
    QTest::addColumn<QString>("xWindowsWMClassName");
    QTest::addColumn<QString>("resultStorageId");

    QTest::addRow("Dolphin") << "org.kde.dolphin" << QString() << "org.kde.dolphin.desktop";
    QTest::addRow("Element (Flatpak)") << "Element" << "element" << "im.riot.Riot.desktop";
    QTest::addRow("Telegram (Flatpak)") << "TelegramDesktop" << "telegram-desktop"
                                        << "org.telegram.desktop.desktop";
    QTest::addRow("Spotify (Flatpak)") << "Spotify" << "spotify"
                                       << "com.spotify.Client.desktop";
    QTest::addRow("GammaRay") << "GammaRay" << "gammary-client" << "GammaRay.desktop";
    QTest::addRow("Gwenview Importer") << "org.kde.gwenview_importer" << "gwenview_importer"
                                       << "org.kde.gwenview_importer.desktop";
    QTest::addRow("kcm_autostart") << "kcm_autostart" << QString() << "kcm_autostart.desktop";
    QTest::addRow("brave") << "Brave-browser" << "brave-browser" << "brave-browser.desktop";
    QTest::addRow("brave_webapp") << "Brave-browser" << "crx_efmjfjelnicpmdcmfikempdhlmainjcb"
                                  << "brave-efmjfjelnicpmdcmfikempdhlmainjcb-Default.desktop";

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QTest::addRow("kcm_kdeconnect") << dataDir + u"/kservices6/kcm_kdeconnect" << QString() << dataDir + u"/kservices6/kcm_kdeconnect.desktop";

    QTest::addRow("Empty appId and xWindowsWMClassName, don't match marisa..desktop") << QString() << QString() << QString();
}

void TaskToolsTest::testServiceFromCmdLine_data()
{
    QTest::addColumn<QString>("cmdLine");
    QTest::addColumn<QString>("processName");
    QTest::addColumn<QString>("serviceName");
    QTest::addColumn<QString>("serviceExec");
    QTest::addColumn<QString>("serviceDesktopName");

    QTest::addRow("gammaray_with_arg") << "gammaray --foo" << "gammaray" << "GammaRay" << "gammaray" << "GammaRay";
    QTest::addRow("gammaray_absolute") << "/usr/bin/gammaray" << "gammaray" << "GammaRay" << "gammaray" << "GammaRay";
    QTest::addRow("no_desktop_file") << "ls -la" << "ls" << "ls" << "ls" << "";
}

void TaskToolsTest::testServiceFromCmdLine()
{
    QFETCH(QString, cmdLine);
    QFETCH(QString, processName);
    QFETCH(QString, serviceName);
    QFETCH(QString, serviceExec);
    QFETCH(QString, serviceDesktopName);

    const auto services = servicesFromCmdLine(cmdLine, processName);

    QCOMPARE(services.size(), 1);
    QCOMPARE(services.first()->name(), serviceName);
    QCOMPARE(services.first()->exec(), serviceExec);
    QCOMPARE(services.first()->desktopEntryName(), serviceDesktopName);
}

void TaskToolsTest::testServiceForUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expectedDesktopName");
    QTest::addColumn<QString>("expectedStorageId");

    QTest::addRow("applicationsUrl") << "applications:org.kde.dolphin.desktop" << "org.kde.dolphin" << "org.kde.dolphin.desktop";

    QTest::addRow("absolute_app") << u"file://" + QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"applications/org.kde.dolphin.desktop"_s)
                                  << "org.kde.dolphin" << "org.kde.dolphin.desktop";

    QTest::addRow("absolute_desktop") << u"file://" + QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"kservices6/kcm_kdeconnect.desktop"_s)
                                      << "kcm_kdeconnect"
                                      << QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"kservices6/kcm_kdeconnect.desktop"_s);

    QTest::addRow("executable") << "file:curl" << "" << "";

    // TODO test preferred:// URL
}

void TaskToolsTest::testServiceForUrl()
{
    QFETCH(QString, url);
    QFETCH(QString, expectedDesktopName);
    QFETCH(QString, expectedStorageId);

    const auto service = serviceForUrl(QUrl(url));
    QCOMPARE(service->desktopEntryName(), expectedDesktopName);
    QCOMPARE(service->storageId(), expectedStorageId);
}

QTEST_MAIN(TaskToolsTest)

#include "tasktoolstest.moc"
