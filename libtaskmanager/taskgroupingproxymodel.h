/********************************************************************
Copyright 2016  Eike Hein <hein.org>

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

#ifndef TASKGROUPINGPROXYMODEL_H
#define TASKGROUPINGPROXYMODEL_H

#include <QAbstractProxyModel>

#include "abstracttasksmodeliface.h"
#include "tasksmodel.h"

#include "taskmanager_export.h"

namespace TaskManager
{

/**
 * @short A proxy tasks model for grouping tasks, forming a tree.
 *
 * This proxy model groups tasks in its source tasks model, forming a tree
 * of tasks. Gouping behavior is influenced by various properties set on
 * the proxy model instance.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT TaskGroupingProxyModel : public QAbstractProxyModel, public AbstractTasksModelIface
{
    Q_OBJECT

    Q_PROPERTY(TasksModel::GroupMode groupMode READ groupMode WRITE setGroupMode NOTIFY groupModeChanged)
    Q_PROPERTY(bool groupDemandingAttention READ groupDemandingAttention WRITE setGroupDemandingAttention
        NOTIFY groupDemandingAttentionChanged)
    Q_PROPERTY(int windowTasksThreshold READ windowTasksThreshold WRITE setWindowTasksThreshold NOTIFY windowTasksThresholdChanged)
    Q_PROPERTY(QStringList blacklistedAppIds READ blacklistedAppIds WRITE setBlacklistedAppIds NOTIFY blacklistedAppIdsChanged)
    Q_PROPERTY(QStringList blacklistedLauncherUrls READ blacklistedLauncherUrls WRITE setBlacklistedLauncherUrls
        NOTIFY blacklistedLauncherUrlsChanged)

public:
    explicit TaskGroupingProxyModel(QObject *parent = nullptr);
    ~TaskGroupingProxyModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &proxyIndex, int role) const override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    /**
     * Returns the current group mode, i.e. the criteria by which tasks should
     * be grouped.
     *
     * Defaults to TasksModel::GroupApplication, which groups tasks backed by
     * the same application.
     *
     * If the group mode is TasksModel::GroupDisabled, no grouping is done.
     *
     * @see TasksModel
     * @see setGroupMode
     * @returns the active group mode.
     **/
    TasksModel::GroupMode groupMode() const;

    /**
     * Sets the group mode, i.e. the criteria by which tasks should be grouped.
     *
     * The group mode can be set to TasksModel::GroupDisabled to disable grouping
     * entirely, breaking apart any existing groups.
     *
     * @see TasksModel
     * @see groupMode
     * @param mode A TasksModel group mode.
     **/
    void setGroupMode(TasksModel::GroupMode mode);

    /**
     * Whether new tasks which demand attention
     * (AbstractTasksModel::IsDemandingAttention) should be grouped immediately,
     * or only once they have stopped demanding attention. Defaults to @c false.
     *
     * @see setGroupDemandingAttention
     * @returns whether tasks which demand attention are grouped immediately.
     **/
    bool groupDemandingAttention() const;

    /**
     * Sets whether new tasks which demand attention
     * (AbstractTasksModel::IsDemandingAttention) should be grouped immediately,
     * or only once they have stopped demanding attention.
     *
     * @see groupDemandingAttention
     * @param group Whether tasks with demand attention should be grouped immediately.
     **/
    void setGroupDemandingAttention(bool group);

    /**
     * As window tasks (AbstractTasksModel::IsWindow) come and go in the source
     * model, groups will be formed when this threshold value is exceeded, and
     * broken apart when it matches or falls below.
     *
     * Defaults to @c -1, which means grouping is done regardless of the number
     * of window tasks in the source model.
     *
     * @see setWindowTasksThreshold
     * @return the threshold number of source window tasks used in grouping
     * decisions.
     **/
    int windowTasksThreshold() const;

    /**
     * Sets the number of source model window tasks (AbstractTasksModel::IsWindow)
     * above which groups will be formed, and at or below which groups will be broken
     * apart.
     *
     * If set to -1, grouping will be done regardless of the number of window tasks
     * in the source model.
     *
     * @see windowTasksThreshold
     * @param threshold A threshold number of source window tasks used in grouping
     * decisions.
     **/
    void setWindowTasksThreshold(int threshold);

    /**
     * A blacklist of app ids (AbstractTasksModel::AppId) that is consulted before
     * grouping a task. If a task's app id is found on the blacklist, it is not
     * grouped.
     *
     * The default app id blacklist is empty.
     *
     * @see setBlacklistedAppIds
     * @returns the blacklist of app ids consulted before grouping a task.
     **/
    QStringList blacklistedAppIds() const;

    /**
     * Sets the blacklist of app ids (AbstractTasksModel::AppId) that is consulted
     * before grouping a task. If a task's app id is found on the blacklist, it is
     * not grouped.
     *
     * When set, groups will be formed and broken apart as necessary.
     *
     * @see blacklistedAppIds
     * @param list a blacklist of app ids to be consulted before grouping a task.
     **/
    void setBlacklistedAppIds(const QStringList &list);

    /**
     * A blacklist of launcher URLs (AbstractTasksModel::LauncherUrl) that is
     * consulted before grouping a task. If a task's launcher URL is found on the
     * blacklist, it is not grouped.
     *
     * The default launcher URL blacklist is empty.
     *
     * @see setBlacklistedLauncherUrls
     * @returns the blacklist of launcher URLs consulted before grouping a task.
     **/
    QStringList blacklistedLauncherUrls() const;

    /**
     * Sets the blacklist of launcher URLs (AbstractTasksModel::LauncherUrl) that
     * is consulted before grouping a task. If a task's launcher URL is found on
     * the blacklist, it is not grouped.
     *
     * When set, groups will be formed and broken apart as necessary.
     *
     * @see blacklistedLauncherUrls
     * @param list a blacklist of launcher URLs to be consulted before grouping a task.
     **/
    void setBlacklistedLauncherUrls(const QStringList &list);

    /**
     * Request activation of the task at the given index. Derived classes are
     * free to interpret the meaning of "activate" themselves depending on
     * the nature and state of the task, e.g. launch or raise a window task.
     *
     * @param index An index in this tasks model.
     **/
    void requestActivate(const QModelIndex &index) override;

    /**
     * Request an additional instance of the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     **/
    void requestNewInstance(const QModelIndex &index) override;

    /**
     * Requests to open the given URLs with the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     * @param urls The URLs to be passed to the application.
     **/
    void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) override;

    /**
     * Request the task at the given index be closed.
     *
     * @param index An index in this tasks model.
     **/
    void requestClose(const QModelIndex &index) override;

    /**
     * Request starting an interactive move for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    void requestMove(const QModelIndex &index) override;

    /**
     * Request starting an interactive resize for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be a
     * no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestResize(const QModelIndex &index) override;

    /**
     * Request toggling the minimized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleMinimized(const QModelIndex &index) override;

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleMaximized(const QModelIndex &index) override;

    /**
     * Request toggling the keep-above state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleKeepAbove(const QModelIndex &index) override;

    /**
     * Request toggling the keep-below state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleKeepBelow(const QModelIndex &index) override;

    /**
     * Request toggling the fullscreen state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleFullScreen(const QModelIndex &index) override;

    /**
     * Request toggling the shaded state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleShaded(const QModelIndex &index) override;

    /**
     * Request entering the window at the given index on the specified virtual desktops,
     * leaving any other desktops.
     *
     * On Wayland, virtual desktop ids are QStrings. On X11, they are uint >0.
     *
     * An empty list has a special meaning: The window is entered on all virtual desktops
     * in the session.
     *
     * On X11, a window can only be on one or all virtual desktops. Therefore, only the
     * first list entry is actually used.
     *
     * On X11, the id 0 has a special meaning: The window is entered on all virtual
     * desktops in the session.
     *
     * @param index An index in this window tasks model.
     * @param desktops A list of virtual desktop ids.
     **/
    void requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops) override;

    /**
     * Request entering the window at the given index on a new virtual desktop,
     * which is created in response to this request.
     *
     * @param index An index in this window tasks model.
     **/
    void requestNewVirtualDesktop(const QModelIndex &index) override;

    /**
     * Request moving the task at the given index to the specified activities.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     * @param activities The new list of activities.
     **/
    void requestActivities(const QModelIndex &index, const QStringList &activities) override;

    /**
     * Request informing the window manager of new geometry for a visual
     * delegate for the task at the given index. The geometry should be in
     * screen coordinates.
     *
     * If the task at the given index is a group parent, the geometry is
     * set for all of its children. If the task at the given index is a
     * group member, the geometry is set for all of its siblings.
     *
     * @param index An index in this tasks model.
     * @param geometry Visual delegate geometry in screen coordinates.
     * @param delegate The delegate. Implementations are on their own with
     * regard to extracting information from this, and should take care to
     * reject invalid objects.
     **/
    void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry,
        QObject *delegate = nullptr) override;

    /**
     * Request toggling whether the task at the given index, along with any
     * tasks matching its kind, should be grouped or not. Task groups will be
     * formed or broken apart as needed, along with affecting future grouping
     * decisions as new tasks appear in the source model.
     *
     * As grouping is toggled for a task, updates are made to the blacklisted*
     * properties of the model instance.
     *
     * @see blacklistedAppIds
     * @see blacklistedLauncherUrls
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleGrouping(const QModelIndex &index);

Q_SIGNALS:
    void groupModeChanged() const;
    void groupDemandingAttentionChanged() const;
    void windowTasksThresholdChanged() const;
    void blacklistedAppIdsChanged() const;
    void blacklistedLauncherUrlsChanged() const;

private:
    class Private;
    QScopedPointer<Private> d;

    Q_PRIVATE_SLOT(d, void sourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last))
    Q_PRIVATE_SLOT(d, void sourceRowsInserted(const QModelIndex &parent, int start, int end))
    Q_PRIVATE_SLOT(d, void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last))
    Q_PRIVATE_SLOT(d, void sourceRowsRemoved(const QModelIndex &parent, int start, int end))
    Q_PRIVATE_SLOT(d, void sourceModelAboutToBeReset())
    Q_PRIVATE_SLOT(d, void sourceModelReset())
    Q_PRIVATE_SLOT(d, void sourceDataChanged(QModelIndex,QModelIndex,QVector<int>))
};

}

#endif
