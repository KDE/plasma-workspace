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

#ifndef TASKSMODEL_H
#define TASKSMODEL_H

#include <QSortFilterProxyModel>

#include "abstracttasksmodeliface.h"

#include "taskmanager_export.h"

namespace TaskManager
{

/**
 * @short A unified tasks model.
 *
 * This model presents tasks sourced from supplied launcher URLs, startup
 * notification data and window data retrieved from the windowing server
 * the host process is connected to. The underlying windowing system is
 * abstracted away.
 *
 * The source data is abstracted into a unified lifecycle for tasks
 * suitable for presentation in a user interface.
 *
 * Matching startup and window tasks replace launcher tasks. Startup
 * tasks are omitted when matching window tasks exist. Tasks that desire
 * not to be shown in a user interface are omitted.
 *
 * Tasks may be filtered, sorted or grouped by setting properties on the
 * model.
 *
 * Tasks may be interacted with by calling methods on the model.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT TasksModel : public QSortFilterProxyModel, public AbstractTasksModelIface
{
    Q_OBJECT

    Q_ENUMS(SortMode)
    Q_ENUMS(GroupMode)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int launcherCount READ launcherCount NOTIFY launcherCountChanged)

    Q_PROPERTY(QStringList launcherList READ launcherList WRITE setLauncherList NOTIFY launcherListChanged)

    Q_PROPERTY(bool anyTaskDemandsAttention READ anyTaskDemandsAttention NOTIFY anyTaskDemandsAttentionChanged)

    Q_PROPERTY(int virtualDesktop READ virtualDesktop WRITE setVirtualDesktop NOTIFY virtualDesktopChanged)
    Q_PROPERTY(int screen READ screen WRITE setScreen NOTIFY screenChanged)
    Q_PROPERTY(QString activity READ activity WRITE setActivity NOTIFY activityChanged)

    Q_PROPERTY(bool filterByVirtualDesktop READ filterByVirtualDesktop WRITE setFilterByVirtualDesktop NOTIFY filterByVirtualDesktopChanged)
    Q_PROPERTY(bool filterByScreen READ filterByScreen WRITE setFilterByScreen NOTIFY filterByScreenChanged)
    Q_PROPERTY(bool filterByActivity READ filterByActivity WRITE setFilterByActivity NOTIFY filterByActivityChanged)
    Q_PROPERTY(bool filterNotMinimized READ filterNotMinimized WRITE setFilterNotMinimized NOTIFY filterNotMinimizedChanged)

    Q_PROPERTY(SortMode sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(bool separateLaunchers READ separateLaunchers WRITE setSeparateLaunchers NOTIFY separateLaunchersChanged)
    Q_PROPERTY(bool launchInPlace READ launchInPlace WRITE setLaunchInPlace NOTIFY launchInPlaceChanged)

    Q_PROPERTY(GroupMode groupMode READ groupMode WRITE setGroupMode NOTIFY groupModeChanged)
    Q_PROPERTY(int groupingWindowTasksThreshold READ groupingWindowTasksThreshold
               WRITE setGroupingWindowTasksThreshold NOTIFY groupingWindowTasksThresholdChanged)
    Q_PROPERTY(QStringList groupingAppIdBlacklist READ groupingAppIdBlacklist
               WRITE setGroupingAppIdBlacklist NOTIFY groupingAppIdBlacklistChanged)
    Q_PROPERTY(QStringList groupingLauncherUrlBlacklist READ groupingLauncherUrlBlacklist
               WRITE setGroupingLauncherUrlBlacklist NOTIFY groupingLauncherUrlBlacklistChanged)

public:
    enum SortMode {
        SortDisabled = 0,   /**< No sorting is done. */
        SortManual,         /**< Tasks can be moved with move() and syncLaunchers(). */
        SortAlpha,          /**< Tasks are sorted alphabetically, by AbstractTasksModel::AppName and Qt::DisplayRole. */
        SortVirtualDesktop, /**< Tasks are sorted by the virtual desktop they are on. */
        SortActivity        /**< Tasks are sorted by the number of tasks on the activities they're on. */
    };

    enum GroupMode {
        GroupDisabled = 0, /**< No grouping is done. */
        GroupApplications  /**< Tasks are grouped by the application backing them. */
    };

    explicit TasksModel(QObject *parent = 0);
    virtual ~TasksModel();

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const; // Invokable.

    /**
     * The number of launcher tasks in the tast list.
     *
     * @returns the number of launcher tasks in the task list.
     **/
    int launcherCount() const;

    /**
     * The list of launcher URLs serialized to strings.
     *
     * @see setLauncherList
     * @returns the list of launcher URLs serialized to strings.
     **/
    QStringList launcherList() const;

    /**
     * Replace the list of launcher URL strings.
     *
     * Invalid or empty URLs will be rejected. Duplicate URLs will be
     * collapsed.
     *
     * @see launcherList
     * @param launchers A list of launcher URL strings.
     **/
    void setLauncherList(const QStringList &launchers);

    /**
     * Returns whether any task in the model currently demands attention
     * (AbstractTasksModel::IsDemandingAttention).
     *
     * @returns whether any task in the model currently demands attention.
     **/
    bool anyTaskDemandsAttention() const;

    /**
     * The number of the virtual desktop used in filtering by virtual
     * desktop. Usually set to the number of the current virtual desktop.
     * Defaults to @c -1.
     *
     * @see setVirtualDesktop
     * @returns the number of the virtual desktop used in filtering.
     **/
    int virtualDesktop() const;

    /**
     * Set the number of the virtual desktop to use in filtering by virtual
     * desktop.
     *
     * If set to  @c -1, filtering by virtual desktop is disabled.
     *
     * @see virtualDesktop
     * @param virtualDesktop A virtual desktop number.
     **/
    void setVirtualDesktop(int virtualDesktop);

    /**
     * The number of the screen used in filtering by screen. Usually
     * set to the number of the current screen. Defaults to @c -1.
     *
     * @see setScreen
     * @returns the number of the screen used in filtering.
     **/
    int screen() const;

    /**
     * Set the number of the screen to use in filtering by screen.
     *
     * If set to @c -1, filtering by screen is disabled.
     *
     * @see screen
     * @param screen A screen number.
     **/
    void setScreen(int screen);

    /**
     * The id of the activity used in filtering by activity. Usually
     * set to the id of the current activity. Defaults to an empty id.
     *
     * FIXME: Implement.
     *
     * @see setActivity
     * @returns the id of the activity used in filtering.
     **/
    QString activity() const;

    /**
     * Set the id of the activity to use in filtering by activity.
     *
     * FIXME: Implement.
     *
     * @see activity
     * @param activity An activity id.
     **/
    void setActivity(const QString &activity);

    /**
     * Whether tasks should be filtered by virtual desktop. Defaults to
     * @c false.
     *
     * Filtering by virtual desktop only happens if a virtual desktop
     * number is set, even if this returns @c true.
     *
     * @see setFilterByVirtualDesktop
     * @see setVirtualDesktop
     * @returns @c true if tasks should be filtered by virtual desktop.
     **/
    bool filterByVirtualDesktop() const;

    /**
     * Set whether tasks should be filtered by virtual desktop.
     *
     * Filtering by virtual desktop only happens if a virtual desktop
     * number is set, even if this is set to @c true.
     *
     * @see filterByVirtualDesktop
     * @see setVirtualDesktop
     * @param filter Whether tasks should be filtered by virtual desktop.
     **/
    void setFilterByVirtualDesktop(bool filter);

    /**
     * Whether tasks should be filtered by screen. Defaults to @c false.
     *
     * Filtering by screen only happens if a screen number is set, even
     * if this returns @c true.
     *
     * @see setFilterByScreen
     * @see setScreen
     * @returns @c true if tasks should be filtered by screen.
     **/
    bool filterByScreen() const;

    /**
     * Set whether tasks should be filtered by screen.
     *
     * Filtering by screen only happens if a screen number is set, even
     * if this is set to @c true.
     *
     * @see filterByScreen
     * @see setScreen
     * @param filter Whether tasks should be filtered by screen.
     **/
    void setFilterByScreen(bool filter);

    /**
     * Whether tasks should be filtered by activity. Defaults to @c false.
     *
     * Filtering by activity only happens if an activity id is set, even
     * if this returns @c true.
     *
     * @see setFilterByActivity
     * @see setActivity
     * @returns @ctrue if tasks should be filtered by activity.
     **/
    bool filterByActivity() const;

    /**
     * Set whether tasks should be filtered by activity. Defaults to
     * @c false.
     *
     * Filtering by virtual desktop only happens if an activity id is set,
     * even if this is set to @c true.
     *
     * @see filterByActivity
     * @see setActivity
     * @param filter Whether tasks should be filtered by activity.
     **/
    void setFilterByActivity(bool filter);

    /**
     * Whether non-minimized tasks should be filtered. Defaults to
     * @c false.
     *
     * @see setFilterNotMinimized
     * @returns @c true if non-minimized tasks should be filtered.
     **/
    bool filterNotMinimized() const;

    /**
     * Set whether non-minimized tasks should be filtered.
     *
     * @see filterNotMinimized
     * @param filter Whether non-minimized tasks should be filtered.
     **/
    void setFilterNotMinimized(bool filter);

    /**
     * The sort mode used in sorting tasks. Defaults to SortAlpha.
     *
     * @see setSortMode
     * @returns the curent sort mode.
     **/
    SortMode sortMode() const;

    /**
     * Sets the sort mode used in sorting tasks.
     *
     * @see sortMode
     * @param mode A sort mode.
     **/
    void setSortMode(SortMode mode);

    // FIXME TODO: Add docs once fully implemented.
    bool separateLaunchers() const;
    void setSeparateLaunchers(bool separate);

    /**
     * Whether window tasks should be sorted as their associated launcher
     * tasks or separately. Defaults to @c false.
     *
     * @see setLaunchInPlace
     * @returns whether window tasks should be sorted as their associated
     * launcher tasks.
     **/
    bool launchInPlace() const;

    /**
     * Sets whether window tasks should be sorted as their associated launcher
     * tasks or separately.
     *
     * @see launchInPlace
     * @param launchInPlace Whether window tasks should be sorted as their
     * associated launcher tasks.
     **/
    void setLaunchInPlace(bool launchInPlace);

    /**
     * Returns the current group mode, i.e. the criteria by which tasks should
     * be grouped.
     *
     * Defaults to TasksModel::GroupApplication, which groups tasks backed by
     * the same application.
     *
     * If the group mode is TasksModel::GroupDisabled, no grouping is done.
     *
     * @see setGroupMode
     * @returns the current group mode.
     **/
    TasksModel::GroupMode groupMode() const;

    /**
     * Sets the group mode, i.e. the criteria by which tasks should be grouped.
     *
     * The group mode can be set to TasksModel::GroupDisabled to disable grouping
     * entirely, breaking apart any existing groups.
     *
     * @see groupMode
     * @param mode A group mode.
     **/
    void setGroupMode(TasksModel::GroupMode mode);

    /**
     * As window tasks (AbstractTasksModel::IsWindow) come and go, groups will
     * be formed when this threshold value is exceeded, and  broken apart when
     * it matches or falls below.
     *
     * Defaults to @c -1, which means grouping is done regardless of the number
     * of window tasks.
     *
     * @see setGroupingWindowTasksThreshold
     * @return the threshold number of window tasks used in grouping decisions.
     **/
    int groupingWindowTasksThreshold() const;

    /**
     * Sets the number of window tasks (AbstractTasksModel::IsWindow) above which
     * groups will be formed, and at or below which groups will be broken apart.
     *
     * If set to -1, grouping will be done regardless of the number of window tasks
     * in the source model.
     *
     * @see groupingWindowTasksThreshold
     * @param threshold A threshold number of window tasks used in grouping
     * decisions.
     **/
    void setGroupingWindowTasksThreshold(int threshold);

    /**
     * A blacklist of app ids (AbstractTasksModel::AppId) that is consulted before
     * grouping a task. If a task's app id is found on the blacklist, it is not
     * grouped.
     *
     * The default app id blacklist is empty.
     *
     * @see setGroupingAppIdBlacklist
     * @returns the blacklist of app ids consulted before grouping a task.
     **/
    QStringList groupingAppIdBlacklist() const;

    /**
     * Sets the blacklist of app ids (AbstractTasksModel::AppId) that is consulted
     * before grouping a task. If a task's app id is found on the blacklist, it is
     * not grouped.
     *
     * When set, groups will be formed and broken apart as necessary.
     *
     * @see groupingAppIdBlacklist
     * @param list a blacklist of app ids to be consulted before grouping a task.
     **/
    void setGroupingAppIdBlacklist(const QStringList &list);

    /**
     * A blacklist of launcher URLs (AbstractTasksModel::LauncherUrl) that is
     * consulted before grouping a task. If a task's launcher URL is found on the
     * blacklist, it is not grouped.
     *
     * The default launcher URL blacklist is empty.
     *
     * @see setGroupingLauncherUrlBlacklist
     * @returns the blacklist of launcher URLs consulted before grouping a task.
     **/
    QStringList groupingLauncherUrlBlacklist() const;

    /**
     * Sets the blacklist of launcher URLs (AbstractTasksModel::LauncherUrl) that
     * is consulted before grouping a task. If a task's launcher URL is found on
     * the blacklist, it is not grouped.
     *
     * When set, groups will be formed and broken apart as necessary.
     *
     * @see groupingLauncherUrlBlacklist
     * @param list a blacklist of launcher URLs to be consulted before grouping a task.
     **/
    void setGroupingLauncherUrlBlacklist(const QStringList &list);

    /**
     * Request adding a launcher with the given URL.
     *
     * If this URL is already in the list, the request will fail. URLs are
     * compared for equality after removing the query string used to hold
     * metadata.
     *
     * @see launcherUrlsMatch
     * @param url A launcher URL.
     * @returns @c true if a launcher was added.
     */
    Q_INVOKABLE bool requestAddLauncher(const QUrl &url);

    /**
     * Request removing the launcher with the given URL.
     *
     * If this URL is already in the list, the request will fail. URLs are
     * compared for equality after removing the query string used to hold
     * metadata.
     *
     * @see launcherUrlsMatch
     * @param url A launcher URL.
     * @returns @c true if the launcher was removed.
     */
    Q_INVOKABLE bool requestRemoveLauncher(const QUrl &url);

    /**
     * Return the position of the launcher with the given URL.
     *
     * URLs are compared for equality after removing the query string used
     * to hold metadata.
     *
     * @see launcherUrlsMatch
     * @param url A launcher URL.
     * @returns @c -1 if no launcher exists for the given URL.
     */
    Q_INVOKABLE int launcherPosition(const QUrl &url) const;

    /**
     * Request activation of the task at the given index. Derived classes are
     * free to interpret the meaning of "activate" themselves depending on
     * the nature and state of the task, e.g. launch or raise a window task.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestActivate(const QModelIndex &index);

    /**
     * Request an additional instance of the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestNewInstance(const QModelIndex &index);

    /**
     * Request the task at the given index be closed.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestClose(const QModelIndex &index);

    /**
     * Request starting an interactive move for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestMove(const QModelIndex &index);

    /**
     * Request starting an interactive resize for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be a
     * no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestResize(const QModelIndex &index);

    /**
     * Request toggling the minimized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleMinimized(const QModelIndex &index);

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleMaximized(const QModelIndex &index);

    /**
     * Request toggling the keep-above state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleKeepAbove(const QModelIndex &index);

    /**
     * Request toggling the keep-below state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleKeepBelow(const QModelIndex &index);

    /**
     * Request toggling the fullscreen state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleFullScreen(const QModelIndex &index);

    /**
     * Request toggling the shaded state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleShaded(const QModelIndex &index);

    /**
     * Request moving the task at the given index to the specified virtual
     * desktop.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     * @param desktop A virtual desktop number.
     **/
    Q_INVOKABLE void requestVirtualDesktop(const QModelIndex &index, qint32 desktop);

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
    Q_INVOKABLE void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry,
        QObject *delegate = nullptr);

    /**
     * Request toggling whether the task at the given index, along with any
     * tasks matching its kind, should be grouped or not. Task groups will be
     * formed or broken apart as needed, along with affecting future grouping
     * decisions as new tasks appear.
     *
     * As grouping is toggled for a task, updates are made to the
     * grouping*Blacklist properties of the model instance.
     *
     * @see groupingAppIdBlacklist
     * @see groupingLauncherUrlBlacklist
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleGrouping(const QModelIndex &index);

    /**
     * Moves a (top-level) task to a new position in the list. The insert
     * position is bounded to the list start and end.
     * syncLaunchers() should be called after a set of move operations to
     * update the launcher list to reflect the new order.
     *
     * @see syncLaunchers
     * @see launcherList
     * @param index An index in this tasks model.
     * @param newPos The new list position to move the task to.
     */
    Q_INVOKABLE bool move(int row, int newPos);

    /**
     * Updates the launcher list to reflect the new order after calls to
     * move(), if needed.
     *
     * @see move
     * @see launcherList
     */
    Q_INVOKABLE void syncLaunchers();

    /**
     * Finds the first active (AbstractTasksModel::IsActive) task in the model
     * and returns its QModelIndex, or a null QModelIndex if no active task is
     * found.
     *
     * @returns the model index for the first active task, if any.
     */
    Q_INVOKABLE QModelIndex activeTask() const;

    /**
     * Given a row in the model, returns a QModelIndex for it. To get an index
     * for a child in a task group, an optional child row may be passed as well.
     *
     * This easier to use from Qt Quick views than QAbstractItemModel::index is.
     *
     * @param row A row index in the model.
     * @param childRow A row index for a child of the task group at the given row.
     * @returns a model index for the task at the given row, or for one of its
     * child tasks.
     */
    Q_INVOKABLE QModelIndex makeModelIndex(int row, int childRow = -1) const;

Q_SIGNALS:
    void countChanged() const;
    void launcherCountChanged() const;
    void launcherListChanged() const;
    void anyTaskDemandsAttentionChanged() const;
    void virtualDesktopChanged() const;
    void screenChanged() const;
    void activityChanged() const;
    void filterByVirtualDesktopChanged() const;
    void filterByScreenChanged() const;
    void filterByActivityChanged() const;
    void filterNotMinimizedChanged() const;
    void sortModeChanged() const;
    void separateLaunchersChanged() const;
    void launchInPlaceChanged() const;
    void groupModeChanged() const;
    void groupingWindowTasksThresholdChanged() const;
    void groupingAppIdBlacklistChanged() const;
    void groupingLauncherUrlBlacklistChanged() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    class Private;
    class TasksModelLessThan;
    friend class TasksModelLessThan;
    QScopedPointer<Private> d;
};

}

#endif
