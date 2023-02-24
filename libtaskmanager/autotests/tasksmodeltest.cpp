/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QQmlApplicationEngine>
#include <QtTest>

#include "abstracttasksmodel.h" // For enums
#include "tasksmodel.h"

using namespace TaskManager;

class TasksModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    /**
     * Task manager open entries jump around when pinned apps are moved
     * in the 'Task Manager' with "Keep launchers separate" option
     * unchecked
     * @see https://bugs.kde.org/444816
     */
    void test_moveBug444816();

    /**
     * Pinned apps with a preferred://[something] URI that resolves to nothing should be hidden
     *
     * @see https://bugs.kde.org/436667
     */
    void test_filterOutInvalidPreferredLaunchers();
};

void TasksModelTest::initTestCase()
{
    QGuiApplication::setQuitOnLastWindowClosed(false);
}

void TasksModelTest::test_moveBug444816()
{
    // Prepare launchers and running tasks
    TasksModel model;

    // Follow required settings in BUG 444816
    model.setGroupMode(TasksModel::GroupDisabled);
    model.setSeparateLaunchers(false);
    model.setSortMode(TasksModel::SortManual);

    QSignalSpy rowInsertedSpy(&model, &TasksModel::rowsInserted);

    int rowCount = model.rowCount();
    QVERIFY(model.launcherList().empty());
    const QUrl launcherUrl = QUrl::fromLocalFile(QFINDTESTDATA("data/applications/GammaRay.desktop"));
    model.setLauncherList(QStringList{launcherUrl.toString()});
    // A launcher is added as expected
    QCOMPARE(model.launcherList().size(), 1);
    QCOMPARE(++rowCount, model.rowCount());

    // Create two new windows
    QVariantMap firstWindowProperties;
    firstWindowProperties.insert(QStringLiteral("title"), QStringLiteral("__testwindow__firstwindow__"));
    QVariantMap secondWindowProperties;
    secondWindowProperties.insert(QStringLiteral("title"), QStringLiteral("__testwindow__secondwindow__"));
    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("windowInitialProperties"), QVariantList{firstWindowProperties, secondWindowProperties});

    QQmlApplicationEngine engine;
    engine.setInitialProperties(initialProperties);
    const QString qmlFileName = QFINDTESTDATA("data/windows/ManyWindows.qml");
    engine.load(qmlFileName);

    // Make sure two new windows have been created
    for (int i = 0; i < initialProperties[QStringLiteral("windowInitialProperties")].toList().size(); ++i) {
        rowInsertedSpy.wait(1000);
    }
    QCOMPARE(++ ++rowCount, model.rowCount());

    // TasksModel now looks like: [Launcher] [...] [__testwindow__firstwindow__] [__testwindow__secondwindow__]
    // This test tries to move [Launcher] to the position between the two tasks
    int launcherRow = -1;
    for (int i = 0; i < model.rowCount(); ++i) {
        if (model.index(i, 0).data(AbstractTasksModel::IsLauncher).toBool()) {
            launcherRow = i;
            break;
        }
    }
    QVERIFY(launcherRow >= 0);

    int firstWindowRow = -1;
    for (int i = 0; i < model.rowCount(); ++i) {
        if (model.index(i, 0).data(Qt::DisplayRole).toString() == firstWindowProperties[QStringLiteral("title")]) {
            firstWindowRow = i;
            break;
        }
    }
    QVERIFY(firstWindowRow >= 0);

    qDebug() << "********* BEGIN Before *********";
    for (int i = 0; i < model.rowCount(); ++i) {
        qDebug() << i << model.index(i, 0).data(Qt::DisplayRole).toString();
    }
    qDebug() << "********* End Before *********";

    QVERIFY(model.move(launcherRow, firstWindowRow));
    QCoreApplication::processEvents();

    // Verify the order
    for (int i = 0; i < model.rowCount(); ++i) {
        if (model.index(i, 0).data(AbstractTasksModel::IsLauncher).toBool()) {
            launcherRow = i;
            break;
        }
    }
    for (int i = 0; i < model.rowCount(); ++i) {
        if (model.index(i, 0).data(Qt::DisplayRole).toString() == firstWindowProperties[QStringLiteral("title")]) {
            firstWindowRow = i;
            break;
        }
    }

    qDebug() << "********* BEGIN After *********";
    for (int i = 0; i < model.rowCount(); ++i) {
        qDebug() << i << model.index(i, 0).data(Qt::DisplayRole).toString();
    }
    qDebug() << "********* END After *********";

    QCOMPARE(firstWindowRow + 1, launcherRow);
}

void TasksModelTest::test_filterOutInvalidPreferredLaunchers()
{
    // Prepare launchers and running tasks
    TasksModel model;

    model.setLauncherList(QStringList{
        "preferred://nonexistent",
    });
    QCOMPARE(model.launcherList().size(), 0);

    const QUrl launcherUrl = QUrl::fromLocalFile(QFINDTESTDATA("data/applications/GammaRay.desktop"));
    model.setLauncherList(QStringList{
        "preferred://nonexistent",
        launcherUrl.toString(),
    });
    QCOMPARE(model.launcherList().size(), 1);
}

QTEST_MAIN(TasksModelTest)

#include "tasksmodeltest.moc"
