/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QAbstractItemModelTester>
#include <QAbstractListModel>
#include <QConcatenateTablesProxyModel>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QTest>

#include "abstracttasksmodel.h"
#include "taskgroupingproxymodel.h"
#include "tasksmodel.h"

using namespace TaskManager;

// Minimal flat source exposing the roles TaskGroupingProxyModel groups on.
class FakeTasksModel : public QAbstractListModel
{
    Q_OBJECT
public:
    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : m_tasks.count();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_tasks.count()) {
            return {};
        }
        switch (role) {
        case AbstractTasksModel::IsWindow:
            return m_tasks.at(index.row()).isWindow;
        case AbstractTasksModel::AppId:
        case AbstractTasksModel::AppName:
        case Qt::DisplayRole:
            return m_tasks.at(index.row()).appId;
        case AbstractTasksModel::IsDemandingAttention:
            return m_tasks.at(index.row()).demandsAttention;
        default:
            return {};
        }
    }

    void addTask(const QString &appId, bool isWindow = true)
    {
        beginInsertRows({}, m_tasks.count(), m_tasks.count());
        m_tasks.append({appId, false, isWindow});
        endInsertRows();
    }

    void removeTask(int row)
    {
        beginRemoveRows({}, row, row);
        m_tasks.remove(row);
        endRemoveRows();
    }

    // Batch insert/remove, as QConcatenateTablesProxyModel does when a whole
    // sub-model appears or vanishes; exercises adjustMap() with count > 1 and the
    // multi-row removal loop.
    void addTasks(const QStringList &appIds, bool isWindow = true)
    {
        if (appIds.isEmpty()) {
            return;
        }
        beginInsertRows({}, m_tasks.count(), m_tasks.count() + appIds.count() - 1);
        for (const QString &appId : appIds) {
            m_tasks.append({appId, false, isWindow});
        }
        endInsertRows();
    }

    void removeTasks(int row, int count)
    {
        count = qMin(count, m_tasks.count() - row);
        if (count <= 0) {
            return;
        }
        beginRemoveRows({}, row, row + count - 1);
        m_tasks.remove(row, count);
        endRemoveRows();
    }

    void toggleAttention(int row)
    {
        m_tasks[row].demandsAttention = !m_tasks[row].demandsAttention;
        const QModelIndex idx = index(row, 0);
        Q_EMIT dataChanged(idx, idx, {AbstractTasksModel::IsDemandingAttention});
    }

    // A window's app identity can change at runtime (late WM_CLASS / appId), which
    // changes appsMatch() results without any structural signal.
    void setAppId(int row, const QString &appId)
    {
        m_tasks[row].appId = appId;
        const QModelIndex idx = index(row, 0);
        Q_EMIT dataChanged(idx, idx, {AbstractTasksModel::AppId, AbstractTasksModel::AppName, Qt::DisplayRole});
    }

private:
    struct Task {
        QString appId;
        bool demandsAttention;
        bool isWindow;
    };
    QList<Task> m_tasks;
};

class TaskGroupingTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void stress();

private:
    static void verifyMapping(TaskGroupingProxyModel *model);
};

// Every proxy index (top-level and child) must map to a valid, in-range, unique
// source row, and together they must cover every source row exactly once.
void TaskGroupingTest::verifyMapping(TaskGroupingProxyModel *model)
{
    QSet<int> seen;
    int mapped = 0;

    const auto check = [&](const QModelIndex &proxyIndex) {
        const QModelIndex sourceIndex = model->mapToSource(proxyIndex);
        QVERIFY(sourceIndex.isValid());
        QCOMPARE(sourceIndex.model(), model->sourceModel());
        QVERIFY(sourceIndex.row() >= 0 && sourceIndex.row() < model->sourceModel()->rowCount());
        QVERIFY(!seen.contains(sourceIndex.row()));
        seen.insert(sourceIndex.row());
        ++mapped;
        proxyIndex.data(Qt::DisplayRole); // must not crash
    };

    for (int r = 0; r < model->rowCount(); ++r) {
        const QModelIndex parent = model->index(r, 0);
        const int children = model->rowCount(parent);
        if (children == 0) {
            check(parent);
        } else {
            for (int c = 0; c < children; ++c) {
                check(model->index(c, 0, parent));
            }
        }
    }

    QCOMPARE(mapped, model->sourceModel()->rowCount());
}

void TaskGroupingTest::stress()
{
    // Mirror the real stack: several source models concatenated (so a change in a
    // non-last sub-model inserts/removes in the *middle* of the row space, the case
    // that actually exercises adjustMap()), then a filter proxy, then grouping.
    constexpr int kSubModels = 3;
    FakeTasksModel sub[kSubModels];
    QConcatenateTablesProxyModel concat;
    for (auto &s : sub) {
        concat.addSourceModel(&s);
    }

    QSortFilterProxyModel filter;
    filter.setSourceModel(&concat);
    filter.setFilterRole(Qt::DisplayRole);
    // Sorting on the (mutable) app id makes the filter emit layoutChanged with index
    // moves when an app id changes, another path into the grouping model.
    filter.setSortRole(Qt::DisplayRole);
    filter.setDynamicSortFilter(true);
    filter.sort(0);

    TaskGroupingProxyModel grouping;
    grouping.setSourceModel(&filter);
    grouping.setGroupMode(TasksModel::GroupApplications);

    // Rigorously validate the grouping model's own row signals and index structure;
    // this catches a begin/end*Rows that does not match the real change, which is how
    // a stale index ends up mapped out of range later.
    QAbstractItemModelTester tester(&grouping, QAbstractItemModelTester::FailureReportingMode::Fatal);

    // Deterministic LCG, reseeded periodically so several independent pseudo-random
    // sequences are exercised over the (bounded) accumulating model state.
    static constexpr quint32 seeds[] = {0x1234abcdu, 0xdeadbeefu, 0x0badf00du, 0x5eed1234u, 0xa5a5a5a5u, 0x0f0f3333u};
    quint32 rng = seeds[0];
    const auto next = [&rng]() {
        rng = rng * 1664525u + 1013904223u;
        return rng >> 8;
    };

    // sub[0] holds non-window tasks (launchers / startup notifiers); the rest hold
    // window tasks, matching the real WindowTasksModel + StartupTasksModel/etc. mix
    // that the QConcatenate concatenates.
    constexpr int seedCount = int(sizeof(seeds) / sizeof(seeds[0]));
    for (int step = 0; step < 1000 * seedCount; ++step) {
        if (step % 1000 == 0) {
            rng = seeds[step / 1000]; // start a fresh pseudo-random sequence
        }
        const int si = next() % kSubModels;
        FakeTasksModel &s = sub[si];
        const bool win = (si != 0);
        int op = next() % 10;
        if (s.rowCount() == 0 && (op == 2 || op == 3 || op == 7 || op == 8)) {
            op = 0; // these ops need a non-empty sub-model
        } else if (s.rowCount() > 15 && (op == 0 || op == 1 || op == 6)) {
            op = 2;
        }
        switch (op) {
        case 0:
        case 1:
            s.addTask(QStringLiteral("app%1").arg(next() % 4), win); // often same app -> forms groups
            break;
        case 2:
            s.removeTask(next() % s.rowCount());
            break;
        case 3:
            s.toggleAttention(next() % s.rowCount()); // exercises the regroup-on-attention path
            break;
        case 4:
            grouping.setGroupMode((next() & 1) ? TasksModel::GroupApplications : TasksModel::GroupDisabled);
            break;
        case 5:
            filter.setFilterRegularExpression((next() & 1) ? QStringLiteral("app[012]") : QString());
            break;
        case 6: {
            // Batch insert of 2-4 tasks, mixing apps so some join groups.
            QStringList batch;
            const int n = 2 + (next() % 3);
            for (int k = 0; k < n; ++k) {
                batch << QStringLiteral("app%1").arg(next() % 4);
            }
            s.addTasks(batch, win);
            break;
        }
        case 7: {
            // Batch remove of 2-4 contiguous rows.
            const int count = 2 + (next() % 3);
            const int row = next() % s.rowCount();
            s.removeTasks(row, count);
            break;
        }
        case 8:
            s.setAppId(next() % s.rowCount(), QStringLiteral("app%1").arg(next() % 4)); // changes grouping key
            break;
        case 9: {
            // Startup -> window handoff across sub-models, in randomized order: a
            // non-window launcher/startup task in sub[0] is replaced by a window task
            // (same app) in a window sub-model, near-simultaneously, as when an app
            // finishes mapping or exits abnormally.
            FakeTasksModel &startup = sub[0];
            FakeTasksModel &window = sub[1 + (next() % (kSubModels - 1))];
            const QString app = QStringLiteral("app%1").arg(next() % 4);
            if (next() & 1) {
                window.addTask(app, true);
                if (startup.rowCount() > 0) {
                    startup.removeTask(next() % startup.rowCount());
                }
            } else {
                if (startup.rowCount() > 0) {
                    startup.removeTask(next() % startup.rowCount());
                }
                window.addTask(app, true);
            }
            break;
        }
        }

        verifyMapping(&grouping);
    }
}

QTEST_GUILESS_MAIN(TaskGroupingTest)

#include "taskgroupingtest.moc"
