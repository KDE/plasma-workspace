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

    // Add our applications
    QFile::copy(QFINDTESTDATA("data/applications/org.kde.dolphin.desktop"), dataDir + QLatin1String("/applications/org.kde.dolphin.desktop"));
    QFile::copy(QFINDTESTDATA("data/applications/org.kde.konversation.desktop"), dataDir + QLatin1String("/applications/org.kde.konversation.desktop"));

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

QTEST_MAIN(TaskToolsTest)

#include "tasktoolstest.moc"
