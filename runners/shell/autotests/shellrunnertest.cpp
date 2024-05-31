/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lonau@gmx.de>
*/

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

#include <KPluginMetaData>
#include <KRunner/AbstractRunnerTest>
#include <KShell>

#include <clocale>

using namespace Qt::StringLiterals;

class ShellRunnerTest : public KRunner::AbstractRunnerTest
{
    Q_OBJECT

private:
    QFileInfo createExecutableFile(const QString &fileName);

private Q_SLOTS:
    void initTestCase();
    void testShellrunnerQueries_data();
    void testShellrunnerQueries();
};

void ShellRunnerTest::initTestCase()
{
    initProperties();
}

void ShellRunnerTest::testShellrunnerQueries()
{
    QFETCH(int, matchCount);
    QFETCH(QString, query);
    QFETCH(QString, expectedCommand);
    QFETCH(QStringList, expectedENVs);

    const auto matches = launchQuery(query);
    QCOMPARE(matches.count(), matchCount);
    if (matchCount == 1) {
        const QVariantList matchData = manager->matches().constFirst().data().toList();
        QCOMPARE(matchData.first().toString(), expectedCommand);
        QCOMPARE(matchData.at(1).toStringList(), expectedENVs);
    }
}

void ShellRunnerTest::testShellrunnerQueries_data()
{
    QTest::addColumn<int>("matchCount");
    QTest::addColumn<QString>("query");
    QTest::addColumn<QString>("expectedCommand");
    QTest::addColumn<QStringList>("expectedENVs");

    // On The BSDs the path can differ, this will give us the absolute path
    const QString executablePath = QStandardPaths::findExecutable(u"true"_s);
    QVERIFY(!executablePath.isEmpty());
    // clang-format off
    QTest::newRow("Should show result with full executable path")
        << 1 << executablePath << executablePath << QStringList{};
    QTest::newRow("Should show result with full executable path and args")
        << 1 << executablePath + u" --help" << executablePath + u" --help" << QStringList{};
    QTest::newRow("Should bot show result for non-existent path")
        << 0 << u"/bin/trueeeeeee"_s << QString() << QStringList{};
    QTest::newRow("Should show result for executable name")
        << 1 << u"true"_s << executablePath << QStringList{};
    QTest::newRow("Should show result for executable name and args")
        << 1 << u"true --help"_s << executablePath + u" --help" << QStringList{};

    QTest::newRow("Should show result for executable and ENV variables")
        << 1 << u"LC_ALL=C true"_s << executablePath << QStringList{u"LC_ALL=C"_s};
    QTest::newRow("Should show result for executable + args and ENV variables")
        << 1 << u"LC_ALL=C true --help"_s << executablePath + u" --help" << QStringList{u"LC_ALL=C"_s};
    QTest::newRow("Should show result for executable and multiple ENV variables")
        << 1 << u"LC_ALL=C TEST=1 true"_s << executablePath << QStringList{u"LC_ALL=C"_s, u"TEST=1"_s};
    QTest::newRow("Should show no result for non-existent executable path and ENV variable")
        << 0 << u"LC_ALL=C /bin/trueeeeeeeeeeee"_s << QString() << QStringList{};

    // Some file we can access with a ~
    const QFileInfo testFile = createExecutableFile(u"test.sh"_s);
    const QString testFilePath = testFile.absoluteFilePath();
    const QString tildePath = KShell::tildeCollapse(testFilePath);

    QTest::newRow("Should show result for full path with tilde")
        << 1 << tildePath << KShell::quoteArg(testFilePath) << QStringList{};
    QTest::newRow("Should show result for full path with tilde and envs")
        << 1 << u"LC_ALL=C " + tildePath << KShell::quoteArg(testFilePath) << QStringList{u"LC_ALL=C"_s};
    QTest::newRow("Should show result for full path with tilde + args and envs")
        << 1 << u"LC_ALL=C " + tildePath + u" --help" << KShell::quoteArg(testFilePath) + u" --help" << QStringList{u"LC_ALL=C"_s};

    // Some file we can access with a ~ and which has a space in its filename
    const QFileInfo testSpaceFile = createExecutableFile(u"test space.sh"_s);
    const QString testSpaceFilePath = testSpaceFile.absoluteFilePath();
    const QString tildeSpacePath = KShell::tildeCollapse(testSpaceFile.absoluteFilePath());

    QTest::newRow("Should show no result for full path with tilde and unquoted space")
            << 0 << tildeSpacePath << QString() << QStringList{};
    QTest::newRow("Should show result for full path with tilde and quoted space")
            << 1 << KShell::quoteArg(tildeSpacePath) << KShell::quoteArg(testSpaceFilePath) << QStringList{};
    QTest::newRow("Should show result for full path with tilde, quoted space and args")
            << 1 << KShell::quoteArg(tildeSpacePath) + u" --help"
            << KShell::joinArgs({testSpaceFilePath, u"--help"_s}) << QStringList{};
    // clang-format on
}

QFileInfo ShellRunnerTest::createExecutableFile(const QString &fileName)
{
    const QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(tmpPath).mkpath(u"."_s);
    QFile testFile(tmpPath + QDir::separator() + fileName);
    testFile.open(QIODevice::WriteOnly);
    testFile.setPermissions(QFile::ExeOwner);
    return QFileInfo(testFile);
}

QTEST_MAIN(ShellRunnerTest)

#include "shellrunnertest.moc"
