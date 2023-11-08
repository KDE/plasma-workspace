/*
    SPDX-FileCopyrightText: 2016-2021 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2022 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QStandardPaths>
#include <QTest>
#include <QThread>

#include <KRunner/AbstractRunnerTest>
#include <KSycoca>

#include <clocale>
#include <optional>
#include <sys/types.h>
#include <unistd.h>

class ServiceRunnerTest : public KRunner::AbstractRunnerTest
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testExcutableExactMatch();
    void testKonsoleVsYakuakeComment();
    void testSystemSettings();
    void testSystemSettings2();
    void testCategories();
    void testJumpListActions();
    void testINotifyUsage();
    void testSpecialArgs();
    void testEnv();
};

void ServiceRunnerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    // Set up applications.menu so that kservice works
    const QString menusDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String{"/menus"};
    QDir(menusDir).removeRecursively();
    QDir(menusDir).mkpath(QStringLiteral("."));
    QFile::copy(QFINDTESTDATA("../../../menu/desktop/plasma-applications.menu"), menusDir + QLatin1String("/applications.menu"));

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

    // Init the KRunner test properties
    initProperties();
}

void ServiceRunnerTest::cleanupTestCase()
{
}

void ServiceRunnerTest::testExcutableExactMatch()
{
    const auto matches = launchQuery(QStringLiteral("Virtual Machine Manager ServiceRunnerTest")); // virt-manager.desktop
    QVERIFY(std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.text() == QLatin1String("Virtual Machine Manager ServiceRunnerTest") && match.relevance() == 1;
    }));
}

void ServiceRunnerTest::testKonsoleVsYakuakeComment()
{
    // Yakuake has konsole mentioned in comment, should be rated lower.
    const auto matches = launchQuery(QStringLiteral("kons"));

    bool konsoleFound = false;
    bool yakuakeFound = false;
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
    manager->matchSessionComplete();
    const auto matches = launchQuery(QStringLiteral("settings"));

    bool systemSettingsFound = false;
    bool foreignSystemSettingsFound = false;
    for (const auto &match : matches) {
        qDebug() << "matched" << match;
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
    const auto matches = launchQuery(QStringLiteral("sy"));

    bool systemSettingsFound = false;
    bool foreignSystemSettingsFound = false;
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

void ServiceRunnerTest::testCategories()
{
    auto matches = launchQuery(QStringLiteral("System"));
    QVERIFY(std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.text() == QLatin1String("Konsole ServiceRunnerTest") && match.relevance() == 0.64;
    }));

    // Multiple categories, this should still match, but now as relevant
    matches = launchQuery(QStringLiteral("System KDE TerminalEmulator"));
    QVERIFY(std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.text() == QLatin1String("Konsole ServiceRunnerTest") && match.relevance() == 0.44;
    }));

    // Multiple categories but at least one doesn't match
    matches = launchQuery(QStringLiteral("System KDE Office"));
    QVERIFY(std::none_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.text() == QLatin1String("Konsole ServiceRunnerTest");
    }));

    // Query too short to match any category
    matches = launchQuery(QStringLiteral("Dumm"));
    QVERIFY(matches.isEmpty());
}

void ServiceRunnerTest::testJumpListActions()
{
    auto matches = launchQuery(QStringLiteral("open a new window")); // org.kde.konsole.desktop
    QVERIFY(std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.text() == QLatin1String("Open a New Window - Konsole ServiceRunnerTest") && match.relevance() == 0.65;
    }));

    matches = launchQuery(QStringLiteral("new window"));
    QVERIFY(std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.text() == QLatin1String("Open a New Window - Konsole ServiceRunnerTest") && match.relevance() == 0.5;
    }));

    matches = launchQuery(QStringLiteral("new windows"));
    QVERIFY(std::none_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.text() == QLatin1String("Open a New Window - Konsole ServiceRunnerTest");
    }));
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
    launchQuery(QStringLiteral("settings"));
    QCOMPARE(inotifyCount(), originalCount);
}

void ServiceRunnerTest::testSpecialArgs()
{
    const auto matches = launchQuery(QStringLiteral("kpat"));
    QVERIFY(std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        // Should have no -qwindowtitle at the end. Because we use DesktopExecParser, we have a "true" as an exec which is available on all systems
        return match.id().endsWith(QLatin1String("/bin/true"));
    }));
}

void ServiceRunnerTest::testEnv()
{
    const auto matches = launchQuery(QStringLiteral("audacity"));
    QVERIFY(std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        // Because we use DesktopExecParser, we have a "true" as an exec which is available on all systems
        return match.id().endsWith(QLatin1String("/bin/true"));
    }));
}

QTEST_MAIN(ServiceRunnerTest)

#include "servicerunnertest.moc"
