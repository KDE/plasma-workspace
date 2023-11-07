/*
    SPDX-FileCopyrightText: 2022 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../runnermatchesmodel.h"
#include "../runnermodel.h"

#include <memory>

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KSycoca>

static const QString s_keyword = QStringLiteral("audacity");

class TestRunnerModel : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testQuery();
    void testMergeResults();
};

void TestRunnerModel::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    QFileInfo desktopFile(QFINDTESTDATA("../../../../runners/services/autotests/fixtures/audacity.desktop"));
    QVERIFY(desktopFile.exists());
    const QString appsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir(appsPath).removeRecursively();
    QVERIFY(QDir().mkpath(appsPath));
    QVERIFY(QFile::copy(desktopFile.absoluteFilePath(), appsPath + QDir::separator() + desktopFile.fileName()));

    QSignalSpy databaseChangedSpy(KSycoca::self(), &KSycoca::databaseChanged);
    KSycoca::self()->ensureCacheValid();
    databaseChangedSpy.wait(2500);

    // Disable all DBus runners, those might cause a slowdown/timeout on CI
    KConfigGroup pluginsConfig = KSharedConfig::openConfig(QStringLiteral("krunnerrc"))->group(QStringLiteral("Plugins"));
    const auto runners = KRunner::RunnerManager::runnerMetaDataList();
    for (const KPluginMetaData &runner : runners) {
        if (runner.value(QStringLiteral("X-Plasma-API")).startsWith(QLatin1String("DBus"))) {
            pluginsConfig.writeEntry(runner.pluginId() + QLatin1String("Enabled"), false);
        }
    }
}

void TestRunnerModel::cleanupTestCase()
{
    const QString appsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir(appsPath).removeRecursively();
    KSharedConfig::openConfig(QStringLiteral("krunnerrc"))->deleteGroup(QStringLiteral("Plugins"));
}

void TestRunnerModel::testQuery()
{
    std::unique_ptr<RunnerModel> model = std::make_unique<RunnerModel>();
    model->setRunners(QStringList{QStringLiteral("krunner_services")});
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(model.get()));
    QSignalSpy countChangedSpy(model.get(), &RunnerModel::countChanged);
    QSignalSpy queryFinishedSpy(model.get(), &RunnerModel::queryFinished);

    // Searching for a really basic string should at least show some results.
    model->setQuery(s_keyword);

    queryFinishedSpy.wait();
    QVERIFY(model->count() > 0);
    QCOMPARE(countChangedSpy.count(), 1);
    QCOMPARE(model->count(), model->rowCount());

    for (int i = 0; i < model->count(); ++i) {
        auto rowModel = qobject_cast<RunnerMatchesModel *>(model->modelForRow(i));
        QVERIFY(rowModel != nullptr);
        QVERIFY(rowModel->rowCount() > 0);
    }

    // If we then change the query to something that shouldn't show anything,
    // the model should not change categories but matches should be gone.
    auto previousCount = model->count();
    model->setQuery(QStringLiteral("something_that_really_shouldn't_return_any_results"));

    queryFinishedSpy.wait();

    QCOMPARE(model->count(), previousCount);
    QCOMPARE(countChangedSpy.count(), 1);

    for (int i = 0; i < model->count(); ++i) {
        auto rowModel = qobject_cast<RunnerMatchesModel *>(model->modelForRow(i));
        QVERIFY(rowModel != nullptr);
        QCOMPARE(rowModel->rowCount(), 0);
    }
}

void TestRunnerModel::testMergeResults()
{
    std::unique_ptr<RunnerModel> model = std::make_unique<RunnerModel>();
    model->setMergeResults(true);
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(model.get()));
    QSignalSpy queryFinished(model.get(), &RunnerModel::queryFinished);

    // A basic query should return some results.
    model->setQuery(s_keyword);
    QCOMPARE(model->count(), 1);
    qWarning() << model->modelForRow(0)->runnerManager()->runners();
    QVERIFY(queryFinished.wait());

    model->setQuery(QString{});
    QVERIFY(queryFinished.wait());
    QCOMPARE(model->modelForRow(0)->count(), 0);
}

QTEST_MAIN(TestRunnerModel)
#include "testrunnermodel.moc"
