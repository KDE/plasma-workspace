/*
    SPDX-FileCopyrightText: 2022 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../runnermatchesmodel.h"
#include "../runnermodel.h"

#include <memory>

#include <QAbstractItemModelTester>
#include <QtTest>

class TestRunnerModel : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testQuery();
    void testDeleteWhenEmpty();
    void testMergeResults();
};

void TestRunnerModel::testQuery()
{
    std::unique_ptr<RunnerModel> model = std::make_unique<RunnerModel>();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(model.get()));
    QSignalSpy countChangedSpy(model.get(), &RunnerModel::countChanged);
    QSignalSpy queryFinishedSpy(model.get(), &RunnerModel::queryFinished);

    // Searching for a really basic string should at least show some results.
    model->setQuery(QStringLiteral("a"));

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

void TestRunnerModel::testDeleteWhenEmpty()
{
    std::unique_ptr<RunnerModel> model = std::make_unique<RunnerModel>();
    model->setDeleteWhenEmpty(true);
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(model.get()));
    QSignalSpy countChangedSpy(model.get(), &RunnerModel::countChanged);

    // Searching for a really basic string should at least show some results.
    model->setQuery(QStringLiteral("a"));
    QTRY_VERIFY(model->count() > 0);
    QCOMPARE(countChangedSpy.count(), 1);
    QCOMPARE(model->count(), model->rowCount());

    // If we then change the query to something that shouldn't show anything,
    // the model should clear with deleteWhenEmpty set.
    model->setQuery(QStringLiteral("something that really shouldn't return any results"));
    QTRY_VERIFY(model->count() == 0);
    QCOMPARE(countChangedSpy.count(), 2);
    QCOMPARE(model->count(), model->rowCount());

    // Repeat the query so the model has some results.
    model->setQuery(QStringLiteral("a"));
    QTRY_VERIFY(model->count() > 0);
    QCOMPARE(countChangedSpy.count(), 3);
    QCOMPARE(model->count(), model->rowCount());

    // Clearing the query should reset the model to empty.
    model->setQuery(QString{});
    QTRY_VERIFY(model->count() == 0);
    QCOMPARE(countChangedSpy.count(), 4);
    QCOMPARE(model->count(), model->rowCount());
}

void TestRunnerModel::testMergeResults()
{
    std::unique_ptr<RunnerModel> model = std::make_unique<RunnerModel>();
    model->setMergeResults(true);
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(model.get()));
    QSignalSpy countChangedSpy(model.get(), &RunnerModel::countChanged);

    // A basic query should return some results.
    model->setQuery(QStringLiteral("a"));
    QTRY_VERIFY(model->count() > 0);
    QCOMPARE(countChangedSpy.count(), 1);
    QCOMPARE(model->count(), model->rowCount());

    model->setQuery(QString{});
    QTRY_VERIFY(model->count() == 0);
}

QTEST_MAIN(TestRunnerModel)
#include "testrunnermodel.moc"
