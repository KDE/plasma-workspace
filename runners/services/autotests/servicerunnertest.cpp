/*
 *   Copyright (C) 2016-2020 Harald Sitter <sitter@kde.org>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) version 3, or any
 *   later version accepted by the membership of KDE e.V. (or its
 *   successor approved by the membership of KDE e.V.), which shall
 *   act as a proxy defined in Section 6 of version 3 of the license.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
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
    void testForeignAppsOutscoreKCMs();
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
        QVERIFY2(QFile::copy(fileInfo.absoluteFilePath(), target),
                 qPrintable(QStringLiteral("can't copy %1 => %2").arg(source, target)));
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
    ServiceRunner runner(this, QVariantList());
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
    ServiceRunner runner(this, QVariantList());
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
    ServiceRunner runner(this, QVariantList());
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

void ServiceRunnerTest::testForeignAppsOutscoreKCMs()
{
    // Our software outscores other things, but foreign applications should still
    // outscore our KCMs.
    ServiceRunner runner(this, QVariantList());
    Plasma::RunnerContext context;
    context.setQuery(QStringLiteral("virt"));

    runner.match(context);

    std::optional<qreal> virtManRelevance;
    std::optional<qreal> virtThingsRelevance;
    std::optional<qreal> kcmRelevance;
    const auto matches = context.matches();
    for (const auto &match : matches) {
        const QUrl url = match.data().toUrl();
        if (url == QUrl(QStringLiteral("applications:virt-manager.desktop"))) {
            virtManRelevance = match.relevance();
        } else if (url == QUrl(QStringLiteral("applications:kcm_kwin_virtualdesktops.desktop"))) {
            kcmRelevance = match.relevance();
        } else if (url == QUrl(QStringLiteral("applications:org.kde.virtthings.desktop"))) {
            virtThingsRelevance = match.relevance();
        }
    }
    QVERIFY(virtManRelevance.has_value());
    QVERIFY(virtThingsRelevance.has_value());
    QVERIFY(kcmRelevance.has_value());

    // KDE app should be >= non-KDE app
    QVERIFY2(virtThingsRelevance >= virtManRelevance,
             qPrintable(QStringLiteral("%1 >= %2").arg(virtThingsRelevance.value()).arg(virtManRelevance.value())));
    // KDE app strictly greater KDE kcm
    QVERIFY2(virtThingsRelevance > kcmRelevance,
             qPrintable(QStringLiteral("%1 > %2").arg(virtThingsRelevance.value()).arg(kcmRelevance.value())));
    // non-KDE app also strictly greater (because it is an app)
    QVERIFY2(virtManRelevance > kcmRelevance,
             qPrintable(QStringLiteral("%1 > %2").arg(virtManRelevance.value()).arg(kcmRelevance.value())));
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
        ServiceRunner runner(nullptr, QVariantList());
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
