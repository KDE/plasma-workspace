/*
 *   Copyright (C) 2016 Harald Sitter <sitter@kde.org>
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

#include <KSycoca>

#include "../servicerunner.h"

class ServiceRunnerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testChromeAppsRelevance();
    void testKonsoleVsYakuakeComment();
};

void ServiceRunnerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    auto appsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir(appsPath).removeRecursively();
    QVERIFY(QDir().mkpath(appsPath));
    auto fixtureDir = QDir(QFINDTESTDATA("fixtures"));
    for(auto fileInfo : fixtureDir.entryInfoList(QDir::Files)) {
        auto source = fileInfo.absoluteFilePath();
        auto target = appsPath + QDir::separator() + fileInfo.fileName();
        QVERIFY2(QFile::copy(fileInfo.absoluteFilePath(), target),
                 qPrintable(QString("can't copy %1 => %2").arg(source, target)));
    }

    setlocale(LC_ALL, "C.utf8");

    KSycoca::self()->ensureCacheValid();
}

void ServiceRunnerTest::cleanupTestCase()
{
}

void ServiceRunnerTest::testChromeAppsRelevance()
{
    ServiceRunner runner(this, QVariantList());
    Plasma::RunnerContext context;
    context.setQuery("chrome");

    runner.match(context);

    bool chromeFound = false;
    bool signalFound = false;
    for (auto match : context.matches()) {
        qDebug() << "matched" << match.text();
        if (!match.text().contains("ServiceRunnerTest")) {
            continue;
        }

        if (match.text() == "Google Chrome ServiceRunnerTest") {
            QCOMPARE(match.relevance(), 0.8);
            chromeFound = true;
        } else if (match.text() == "Signal ServiceRunnerTest") {
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
    context.setQuery("kons");

    runner.match(context);

    bool konsoleFound = false;
    bool yakuakeFound = false;
    for (auto match : context.matches()) {
        qDebug() << "matched" << match.text();
        if (!match.text().contains("ServiceRunnerTest")) {
            continue;
        }

        if (match.text() == "Konsole ServiceRunnerTest") {
            QCOMPARE(match.relevance(), 0.99);
            konsoleFound = true;
        } else if (match.text() == "Yakuake ServiceRunnerTest") {
            // Rates lower because it doesn't have it in the name.
            QCOMPARE(match.relevance(), 0.59);
            yakuakeFound = true;
        }
    }
    QVERIFY(konsoleFound);
    QVERIFY(yakuakeFound);
}

QTEST_MAIN(ServiceRunnerTest)

#include "servicerunnertest.moc"
