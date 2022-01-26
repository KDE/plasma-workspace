/*
    SPDX-FileCopyrightText: 2016-2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QStandardPaths>
#include <QTest>
#include <QThread>

#include <KSycoca>

#include "../servicerunner.h"

#include <clocale>
#include <optional>
#include <sys/types.h>
#include <unistd.h>

class ServiceRunnerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testChromeAppsRelevance();
    void testKonsoleVsYakuakeComment();
    void testSystemSettings();
    void testSystemSettings2();
    void testINotifyUsage();
};

void ServiceRunnerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    auto appsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir(appsPath).removeRecursively();
    QVERIFY(QDir().mkpath(appsPath));
    auto fixtureDir = QDir(QFINDTESTDATA("fixtures"));
    const auto infoList = fixtureDir.entryInfoList(QDir::Files);
    for (const auto &fileInfo : infoList) {
        auto source = fileInfo.absoluteFilePath();
        auto target = appsPath + QDir::separator() + fileInfo.fileName();
        QVERIFY2(QFile::copy(fileInfo.absoluteFilePath(), target), qPrintable(QStringLiteral("can't copy %1 => %2").arg(source, target)));
    }

    setlocale(LC_ALL, "C.utf8");

    KSycoca::self()->ensureCacheValid();

    // Make sure noDisplay behaves consistently WRT OnlyShowIn etc.
    QVERIFY(setenv("XDG_CURRENT_DESKTOP", "KDE", 1) == 0);
    // NOTE: noDisplay also includes X-KDE-OnlyShowOnQtPlatforms which is a bit harder to fake
    //       and not currently under testing anyway.
}

void ServiceRunnerTest::cleanupTestCase()
{
}

void ServiceRunnerTest::testChromeAppsRelevance()
{
    ServiceRunner runner(this, KPluginMetaData(), QVariantList());
    Plasma::RunnerContext context;
    context.setQuery(QStringLiteral("chrome"));

    runner.match(context);

    bool chromeFound = false;
    bool signalFound = false;
    const auto matches = context.matches();
    for (const auto &match : matches) {
        qDebug() << "matched" << match.text();
        if (!match.text().contains(QLatin1String("ServiceRunnerTest"))) {
            continue;
        }

        if (match.text() == QLatin1String("Google Chrome ServiceRunnerTest")) {
            QCOMPARE(match.relevance(), 0.8);
            chromeFound = true;
        } else if (match.text() == QLatin1String("Signal ServiceRunnerTest")) {
            // Rates lower because it doesn't have it in the name.
            QCOMPARE(match.relevance(), 0.7);
            signalFound = true;
        }
    }
    QVERIFY(chromeFound);
    QVERIFY(signalFound);
}

void ServiceRunnerTest::testKonsoleVsYakuakeComment()
{
    // Yakuake has konsole mentioned in comment, should be rated lower.
    ServiceRunner runner(this, KPluginMetaData(), QVariantList());
    Plasma::RunnerContext context;
    context.setQuery(QStringLiteral("kons"));

    runner.match(context);

    bool konsoleFound = false;
    bool yakuakeFound = false;
    const auto matches = context.matches();
    for (const auto &match : matches) {
        qDebug() << "matched" << match.text();
        if (!match.text().contains(QLatin1String("ServiceRunnerTest"))) {
            continue;
        }

        if (match.text() == QLatin1String("Konsole ServiceRunnerTest")) {
            QCOMPARE(match.relevance(), 0.99);
            konsoleFound = true;
        } else if (match.text() == QLatin1String("Yakuake ServiceRunnerTest")) {
            // Rates lower because it doesn't have it in the name.
            QCOMPARE(match.relevance(), 0.59);
            yakuakeFound = true;
        }
    }
    QVERIFY(konsoleFound);
    QVERIFY(yakuakeFound);
}

void ServiceRunnerTest::testSystemSettings()
{
    // In 5.9.0 'System Settings' suddenly didn't come back as a match for 'settings' anymore.
    // System Settings has a noKDE version and a KDE version, if the noKDE version is encountered
    // first it will be added to the seen cache, however disqualification of already seen items
    // may then also disqualify the KDE version of system settings on account of having already
    // seen it. This test makes sure we find the right version.
    ServiceRunner runner(this, KPluginMetaData(), QVariantList());
    Plasma::RunnerContext context;
    context.setQuery(QStringLiteral("settings"));

    runner.match(context);

    bool systemSettingsFound = false;
    bool foreignSystemSettingsFound = false;
    const auto matches = context.matches();
    for (const auto &match : matches) {
        qDebug() << "matched" << match.text();
        if (match.text() == QLatin1String("System Settings ServiceRunnerTest")) {
            systemSettingsFound = true;
        }
        if (match.text() == QLatin1String("KDE System Settings ServiceRunnerTest")) {
            foreignSystemSettingsFound = true;
        }
    }
    QVERIFY(systemSettingsFound);
    QVERIFY(!foreignSystemSettingsFound);
}

void ServiceRunnerTest::testSystemSettings2()
{
    ServiceRunner runner(this, KPluginMetaData(), QVariantList());
    Plasma::RunnerContext context;
    context.setQuery(QStringLiteral("sy"));

    runner.match(context);

    bool systemSettingsFound = false;
    bool foreignSystemSettingsFound = false;
    const auto matches = context.matches();
    for (const auto &match : matches) {
        qDebug() << "matched" << match.text();
        if (match.text() == QLatin1String("System Settings ServiceRunnerTest")) {
            systemSettingsFound = true;
        }
        if (match.text() == QLatin1String("KDE System Settings ServiceRunnerTest")) {
            foreignSystemSettingsFound = true;
        }
    }
    QVERIFY(systemSettingsFound);
    QVERIFY(!foreignSystemSettingsFound);
}

void ServiceRunnerTest::testINotifyUsage()
{
    auto inotifyCount = []() -> uint {
        uint count = 0;
        const QDir procDir(QStringLiteral("/proc/%1/fd").arg(getpid()));
        for (const auto &fileInfo : procDir.entryInfoList()) {
            if (fileInfo.symLinkTarget().endsWith(QStringLiteral("anon_inode:inotify"))) {
                ++count;
            }
        }
        return count;
    };

    const uint originalCount = inotifyCount();

    // We'll run this in a new thread so KDirWatch would be led to create a new thread-local watch instance.
    // The expectation here is that this KDW instance is not persistently claiming an inotify instance.
    bool inotifyCountCool = false;
    auto thread = QThread::create([&] {
        ServiceRunner runner(nullptr, KPluginMetaData(), QVariantList());
        Plasma::RunnerContext context;
        context.setQuery(QStringLiteral("settings"));

        runner.match(context);

        QCOMPARE(inotifyCount(), originalCount);
        inotifyCountCool = true;
    });
    thread->start();
    thread->wait();
    thread->deleteLater();

    QVERIFY(inotifyCountCool);
}

QTEST_MAIN(ServiceRunnerTest)

#include "servicerunnertest.moc"
