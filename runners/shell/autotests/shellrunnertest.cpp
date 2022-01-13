/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lonau@gmx.de>
*/

// See https://phabricator.kde.org/T14499, this plugin's id should be renamed
#undef KRUNNER_TEST_RUNNER_PLUGIN_NAME
#define KRUNNER_TEST_RUNNER_PLUGIN_NAME "shell"

#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

#include <KPluginMetaData>
#include <KRunner/AbstractRunnerTest>
#include <KShell>
#include <QSignalSpy>
#include <QStandardPaths>

#include <clocale>

class ShellRunnerTest : public AbstractRunnerTest
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

    launchQuery(query);
    QCOMPARE(manager->matches().count(), matchCount);
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
    const QString executablePath = QStandardPaths::findExecutable("true");
    QVERIFY(!executablePath.isEmpty());
    // clang-format off
    QTest::newRow("Should show result with full executable path")
        << 1 << executablePath << executablePath << QStringList{};
    QTest::newRow("Should show result with full executable path and args")
        << 1 << executablePath + " --help" << executablePath + " --help" << QStringList{};
    QTest::newRow("Should bot show result for non-existent path")
        << 0 << "/bin/trueeeeeee" << QString() << QStringList{};
    QTest::newRow("Should show result for executable name")
        << 1 << "true" << executablePath << QStringList{};
    QTest::newRow("Should show result for executable name and args")
        << 1 << "true --help" << executablePath + " --help" << QStringList{};

    QTest::newRow("Should show result for executable and ENV variables")
        << 1 << "LC_ALL=C true" << executablePath << QStringList{"LC_ALL=C"};
    QTest::newRow("Should show result for executable + args and ENV variables")
        << 1 << "LC_ALL=C true --help" << executablePath + " --help" << QStringList{"LC_ALL=C"};
    QTest::newRow("Should show result for executable and multiple ENV variables")
        << 1 << "LC_ALL=C TEST=1 true" << executablePath << QStringList{"LC_ALL=C", "TEST=1"};
    QTest::newRow("Should show no result for non-existent executable path and ENV variable")
        << 0 << "LC_ALL=C /bin/trueeeeeeeeeeee" << "" << QStringList{};

    // Some file we can access with a ~
    const QFileInfo testFile = createExecutableFile("test.sh");
    const QString testFilePath = testFile.absoluteFilePath();
    const QString tildePath = KShell::tildeCollapse(testFilePath);

    QTest::newRow("Should show result for full path with tilde")
        << 1 << tildePath << KShell::quoteArg(testFilePath) << QStringList{};
    QTest::newRow("Should show result for full path with tilde and envs")
        << 1 << "LC_ALL=C " + tildePath << KShell::quoteArg(testFilePath) << QStringList{"LC_ALL=C"};
    QTest::newRow("Should show result for full path with tilde + args and envs")
        << 1 << "LC_ALL=C " + tildePath + " --help" << KShell::quoteArg(testFilePath) + " --help" << QStringList{"LC_ALL=C"};

    // Some file we can access with a ~ and which has a space in its filename
    const QFileInfo testSpaceFile = createExecutableFile("test space.sh");
    const QString testSpaceFilePath = testSpaceFile.absoluteFilePath();
    const QString tildeSpacePath = KShell::tildeCollapse(testSpaceFile.absoluteFilePath());

    QTest::newRow("Should show no result for full path with tilde and unquoted space")
            << 0 << tildeSpacePath << QString() << QStringList{};
    QTest::newRow("Should show result for full path with tilde and quoted space")
            << 1 << KShell::quoteArg(tildeSpacePath) << KShell::quoteArg(testSpaceFilePath) << QStringList{};
    QTest::newRow("Should show result for full path with tilde, quoted space and args")
            << 1 << KShell::quoteArg(tildeSpacePath) + " --help"
            << KShell::joinArgs({testSpaceFilePath, "--help"}) << QStringList{};
    // clang-format on
}

QFileInfo ShellRunnerTest::createExecutableFile(const QString &fileName)
{
    const QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(tmpPath).mkpath(".");
    QFile testFile(tmpPath + "/" + fileName);
    testFile.open(QIODevice::WriteOnly);
    testFile.setPermissions(QFile::ExeOwner);
    return QFileInfo(testFile);
}

QTEST_MAIN(ShellRunnerTest)

#include "shellrunnertest.moc"
