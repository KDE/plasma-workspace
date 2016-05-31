/********************************************************************
Copyright 2016  Eike Hein <hein@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "taskfilterproxymodel.h"
#include "abstracttasksmodel.h"

namespace TaskManager
{

class TaskFilterProxyModel::Private
{
public:
    Private(TaskFilterProxyModel *q);

    AbstractTasksModelIface *sourceTasksModel = nullptr;

    uint virtualDesktop = 0;
    int screen = -1;
    QString activity;

    bool filterByVirtualDesktop = false;
    bool filterByScreen = false;
    bool filterByActivity = false;
    bool filterNotMinimized = false;

private:
    TaskFilterProxyModel *q;
};

TaskFilterProxyModel::Private::Private(TaskFilterProxyModel *q)
    : q(q)
{
}

TaskFilterProxyModel::TaskFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private(this))
{
}

TaskFilterProxyModel::~TaskFilterProxyModel()
{
}

void TaskFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    d->sourceTasksModel = dynamic_cast<AbstractTasksModelIface *>(sourceModel);

    QSortFilterProxyModel::setSourceModel(sourceModel);
}

uint TaskFilterProxyModel::virtualDesktop() const
{
    return d->virtualDesktop;
}

void TaskFilterProxyModel::setVirtualDesktop(uint virtualDesktop)
{
    if (d->virtualDesktop != virtualDesktop) {
        d->virtualDesktop = virtualDesktop;

        if (d->filterByVirtualDesktop) {
            invalidateFilter();
        }

        emit virtualDesktopChanged();
    }
}

int TaskFilterProxyModel::screen() const
{
    return d->screen;
}

void TaskFilterProxyModel::setScreen(int screen)
{
    if (d->screen != screen) {
        d->screen = screen;

        if (d->filterByScreen) {
            invalidateFilter();
        }

        emit screenChanged();
    }
}

QString TaskFilterProxyModel::activity() const
{
    return d->activity;
}

void TaskFilterProxyModel::setActivity(const QString &activity)
{
    if (d->activity != activity) {
        d->activity = activity;

        if (d->filterByActivity) {
            invalidateFilter();
        }

        emit activityChanged();
    }
}

bool TaskFilterProxyModel::filterByVirtualDesktop() const
{
    return d->filterByVirtualDesktop;
}

void TaskFilterProxyModel::setFilterByVirtualDesktop(bool filter)
{
    if (d->filterByVirtualDesktop != filter) {
        d->filterByVirtualDesktop = filter;

        invalidateFilter();

        emit filterByVirtualDesktopChanged();
    }
}

bool TaskFilterProxyModel::filterByScreen() const
{
    return d->filterByScreen;
}

void TaskFilterProxyModel::setFilterByScreen(bool filter)
{
    if (d->filterByScreen != filter) {
        d->filterByScreen = filter;

        invalidateFilter();

        emit filterByScreenChanged();
    }
}

bool TaskFilterProxyModel::filterByActivity() const
{
    return d->filterByActivity;
}

void TaskFilterProxyModel::setFilterByActivity(bool filter)
{
    if (d->filterByActivity != filter) {
        d->filterByActivity = filter;

        invalidateFilter();

        emit filterByActivityChanged();
    }
}

bool TaskFilterProxyModel::filterNotMinimized() const
{
    return d->filterNotMinimized;
}

void TaskFilterProxyModel::setFilterNotMinimized(bool filter)
{
    if (d->filterNotMinimized != filter) {
        d->filterNotMinimized = filter;

        invalidateFilter();

        emit filterNotMinimizedChanged();
    }
}

void TaskFilterProxyModel::requestActivate(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestActivate(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestNewInstance(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestNewInstance(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestClose(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestClose(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestMove(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestMove(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestResize(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestResize(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestToggleMinimized(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleMinimized(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestToggleMaximized(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleMaximized(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleKeepAbove(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleKeepBelow(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleFullScreen(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestToggleShaded(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleShaded(mapToSource(index));
    }
}

void TaskFilterProxyModel::requestVirtualDesktop(const QModelIndex &index, qint32 desktop)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestVirtualDesktop(mapToSource(index), desktop);
    }
}

void TaskFilterProxyModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    if (index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestPublishDelegateGeometry(mapToSource(index), geometry, delegate);
    }
}

bool TaskFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    const QModelIndex &sourceIdx = sourceModel()->index(sourceRow, 0);

    // Filter tasks that are not to be shown on the task bar.
    if (sourceIdx.data(AbstractTasksModel::SkipTaskbar).toBool()) {
        return false;
    }

    // Filter by virtual desktop.
    if (d->filterByVirtualDesktop && d->virtualDesktop != 0) {
        if (!sourceIdx.data(AbstractTasksModel::IsOnAllVirtualDesktops).toBool()) {
            const QVariant &virtualDesktop = sourceIdx.data(AbstractTasksModel::VirtualDesktop);

            if (!virtualDesktop.isNull()) {
                bool ok = false;
                const uint i = virtualDesktop.toUInt(&ok);

                if (ok && i != d->virtualDesktop) {
                    return false;
                }
            }
        }
    }

    // Filter by screen.
    if (d->filterByScreen && d->screen != -1) {
        const QVariant &screen = sourceIdx.data(AbstractTasksModel::Screen);

        if (!screen.isNull()) {
            bool ok = false;
            const int i = screen.toInt(&ok);

            if (ok && i != -1 && i != d->screen) {
                return false;
            }
        }
    }

    // Filter by activity.
    if (d->filterByActivity && !d->activity.isEmpty()) {
        const QVariant &activities = sourceIdx.data(AbstractTasksModel::Activities);

        if (!activities.isNull()) {
            const QStringList l = activities.toStringList();

            if (!l.isEmpty() && !l.contains(d->activity)) {
                return false;
            }
        }
    }

    // Filter not minimized.
    if (d->filterNotMinimized) {
        bool isMinimized = sourceIdx.data(AbstractTasksModel::IsMinimized).toBool();

        if (!isMinimized) {
            return false;
        }
    }

    return true;
}

}
