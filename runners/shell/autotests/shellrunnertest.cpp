
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

#include <KShell>
#include <KPluginLoader>
#include <KPluginMetaData>
#include <KRunner/RunnerManager>
#include <QSignalSpy>

using namespace Plasma;

class ShellRunnerTest : public QObject
{
Q_OBJECT

private:
    RunnerManager *manager = nullptr;

private Q_SLOTS:
    void initTestCase();
    void testShellrunnerQueries_data();
    void testShellrunnerQueries();
};

void ShellRunnerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    setlocale(LC_ALL, "C.utf8");

    auto pluginMetaDatas = KPluginLoader::findPluginsById(QStringLiteral(PLUGIN_BUILD_DIR), QStringLiteral(RUNNER_NAME));
    QCOMPARE(pluginMetaDatas.count(), 1);
    KPluginMetaData runnerMetadata = pluginMetaDatas.first();
    delete manager;
    manager = new RunnerManager();
    manager->setAllowedRunners({QStringLiteral(RUNNER_NAME)});
    manager->loadRunner(runnerMetadata);
    QCOMPARE(manager->runners().count(), 1);
}

void ShellRunnerTest::testShellrunnerQueries()
{
    QFETCH(int, matchCount);
    QFETCH(QString, query);
    QFETCH(QString, expectedCommand);
    QFETCH(QStringList, expectedENVs);

    QSignalSpy spy(manager, &RunnerManager::queryFinished);
    manager->launchQuery(query);
    QVERIFY(spy.wait());
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

    QTest::newRow("Should show result with full executable path")
        << 1 << "/bin/true" << "/bin/true" << QStringList{};
    QTest::newRow("Should show result with full executable path and args")
        << 1 << "/bin/true --help" << "/bin/true --help" << QStringList{};
    QTest::newRow("Should bot show result for non-existent path")
        << 0 << "/bin/trueeeeeee" << QString() << QStringList{};
    QTest::newRow("Should show result for executable name")
        << 1 << "true" << "true" << QStringList{};
    QTest::newRow("Should show result for executable name and args")
        << 1 << "true --help" << "true --help" << QStringList{};

    QTest::newRow("Should show result for executable and ENV variables")
        << 1 << "LC_ALL=C true" << "true" << QStringList{"LC_ALL=C"};
    QTest::newRow("Should show result for executable + args and ENV variables")
        << 1 << "LC_ALL=C true --help" << "true --help" << QStringList{"LC_ALL=C"};
    QTest::newRow("Should show result for executable and multiple ENV variables")
        << 1 << "LC_ALL=C TEST=1 true" << "true" << QStringList{"LC_ALL=C", "TEST=1"};
    QTest::newRow("Should show no result for non-existent executable path and ENV variable")
        << 0 << "LC_ALL=C /bin/trueeeeeeeeeeee" << "" << QStringList{};

    // Some file we can access with a ~
    const QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(tmpPath).mkpath(".");
    QFile testFile(tmpPath + "/test.sh");
    testFile.open(QIODevice::WriteOnly);
    testFile.setPermissions(QFile::ExeOwner);
    const QString absoluteFilePath = testFile.fileName();
    const QString tildePath = KShell::tildeCollapse(absoluteFilePath);

    QTest::newRow("Should show result for full path with tilde")
        << 1 << tildePath << tildePath << QStringList{};
    QTest::newRow("Should show result for full path with tilde and envs")
        << 1 << "LC_ALL=C " + tildePath << KShell::quoteArg(tildePath) << QStringList{"LC_ALL=C"};
    QTest::newRow("Should show result for full path with tilde + args and envs")
        << 1 << "LC_ALL=C " + tildePath + " --help" << KShell::quoteArg(tildePath) + " --help" << QStringList{"LC_ALL=C"};
}

QTEST_MAIN(ShellRunnerTest)

#include "shellrunnertest.moc"
