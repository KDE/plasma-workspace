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

#include <QQmlParserStatus>
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

class TASKMANAGER_EXPORT TasksModel : public QSortFilterProxyModel, public AbstractTasksModelIface, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int launcherCount READ launcherCount NOTIFY launcherCountChanged)

    Q_PROPERTY(QStringList launcherList READ launcherList WRITE setLauncherList NOTIFY launcherListChanged)

    Q_PROPERTY(bool anyTaskDemandsAttention READ anyTaskDemandsAttention NOTIFY anyTaskDemandsAttentionChanged)

    Q_PROPERTY(QVariant virtualDesktop READ virtualDesktop WRITE setVirtualDesktop NOTIFY virtualDesktopChanged)
    Q_PROPERTY(QRect screenGeometry READ screenGeometry WRITE setScreenGeometry NOTIFY screenGeometryChanged)
    Q_PROPERTY(QString activity READ activity WRITE setActivity NOTIFY activityChanged)

    Q_PROPERTY(bool filterByVirtualDesktop READ filterByVirtualDesktop WRITE setFilterByVirtualDesktop NOTIFY filterByVirtualDesktopChanged)
    Q_PROPERTY(bool filterByScreen READ filterByScreen WRITE setFilterByScreen NOTIFY filterByScreenChanged)
    Q_PROPERTY(bool filterByActivity READ filterByActivity WRITE setFilterByActivity NOTIFY filterByActivityChanged)
    Q_PROPERTY(bool filterNotMinimized READ filterNotMinimized WRITE setFilterNotMinimized NOTIFY filterNotMinimizedChanged)

    Q_PROPERTY(SortMode sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(bool separateLaunchers READ separateLaunchers WRITE setSeparateLaunchers NOTIFY separateLaunchersChanged)
    Q_PROPERTY(bool launchInPlace READ launchInPlace WRITE setLaunchInPlace NOTIFY launchInPlaceChanged)

    Q_PROPERTY(GroupMode groupMode READ groupMode WRITE setGroupMode NOTIFY groupModeChanged)
    Q_PROPERTY(bool groupInline READ groupInline WRITE setGroupInline NOTIFY groupInlineChanged)
    Q_PROPERTY(int groupingWindowTasksThreshold READ groupingWindowTasksThreshold
               WRITE setGroupingWindowTasksThreshold NOTIFY groupingWindowTasksThresholdChanged)
    Q_PROPERTY(QStringList groupingAppIdBlacklist READ groupingAppIdBlacklist
               WRITE setGroupingAppIdBlacklist NOTIFY groupingAppIdBlacklistChanged)
    Q_PROPERTY(QStringList groupingLauncherUrlBlacklist READ groupingLauncherUrlBlacklist
               WRITE setGroupingLauncherUrlBlacklist NOTIFY groupingLauncherUrlBlacklistChanged)
    Q_PROPERTY(QModelIndex activeTask READ activeTask NOTIFY activeTaskChanged)

public:
    enum SortMode {
        SortDisabled = 0,   /**< No sorting is done. */
        SortManual,         /**< Tasks can be moved with move() and syncLaunchers(). */
        SortAlpha,          /**< Tasks are sorted alphabetically, by AbstractTasksModel::AppName and Qt::DisplayRole. */
        SortVirtualDesktop, /**< Tasks are sorted by the virtual desktop they are on. */
        SortActivity        /**< Tasks are sorted by the number of tasks on the activities they're on. */
    };
    Q_ENUM(SortMode)

    enum GroupMode {
        GroupDisabled = 0, /**< No grouping is done. */
        GroupApplications  /**< Tasks are grouped by the application backing them. */
    };
    Q_ENUM(GroupMode)

    explicit TasksModel(QObject *parent = nullptr);
    ~TasksModel() override;

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override; // Invokable.

    QVariant data(const QModelIndex &proxyIndex, int role) const override;

    /**
     * The number of launcher tasks in the tast list.
     *
     * @returns the number of launcher tasks in the task list.
     **/
    int launcherCount() const;

    /**
     * The list of launcher URLs serialized to strings along with
     * the activities they belong to.
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
     * The id of the virtual desktop used in filtering by virtual
     * desktop. Usually set to the id of the current virtual desktop.
     * Defaults to empty.
     *
     * @see setVirtualDesktop
     * @returns the number of the virtual desktop used in filtering.
     **/
    QVariant virtualDesktop() const;

    /**
     * Set the id of the virtual desktop to use in filtering by virtual
     * desktop.
     *
     * If set to an empty id, filtering by virtual desktop is disabled.
     *
     * @see virtualDesktop
     * @param desktop A virtual desktop id (QString on Wayland; uint >0 on X11).
     **/
    void setVirtualDesktop(const QVariant &desktop = QVariant());

    /**
     * The geometry of the screen used in filtering by screen. Defaults
     * to a null QRect.
     *
     * @see setGeometryScreen
     * @returns the geometry of the screen used in filtering.
     **/
    QRect screenGeometry() const;

    /**
     * Set the geometry of the screen to use in filtering by screen.
     *
     * If set to an invalid QRect, filtering by screen is disabled.
     *
     * @see screenGeometry
     * @param geometry A screen geometry.
     **/
    void setScreenGeometry(const QRect &geometry);

    /**
     * The id of the activity used in filtering by activity. Usually
     * set to the id of the current activity. Defaults to an empty id.
     *
     * @see setActivity
     * @returns the id of the activity used in filtering.
     **/
    QString activity() const;

    /**
     * Set the id of the activity to use in filtering by activity.
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
     * @returns the current sort mode.
     **/
    SortMode sortMode() const;

    /**
     * Sets the sort mode used in sorting tasks.
     *
     * @see sortMode
     * @param mode A sort mode.
     **/
    void setSortMode(SortMode mode);

    /**
     * Whether launchers are kept separate from other kinds of tasks.
     * Defaults to @c true.
     *
     * When enabled, launcher tasks are sorted first in the tasks model
     * and move() disallows moving them below the last launcher task,
     * or moving a different kind of task above the first launcher. New
     * launcher tasks are inserted after the last launcher task. When
     * disabled, move() allows mixing, and new launcher tasks are
     * appended to the model.
     *
     * Further, when disabled, the model always behaves as if
     * launchInPlace is enabled: A window task takes the place of the
     * first matching launcher task.
     *
     * @see LauncherTasksModel
     * @see move
     * @see launchInPlace
     * @see setSeparateLaunchers
     * @return whether launcher tasks are kept separate.
     */
    bool separateLaunchers() const;

    /**
     * Sets whether launchers are kept separate from other kinds of tasks.
     *
     * When enabled, launcher tasks are sorted first in the tasks model
     * and move() disallows moving them below the last launcher task,
     * or moving a different kind of task above the first launcher. New
     * launcher tasks are inserted after the last launcher task. When
     * disabled, move() allows mixing, and new launcher tasks are
     * appended to the model.
     *
     * Further, when disabled, the model always behaves as if
     * launchInPlace is enabled: A window task takes the place of the
     * first matching launcher task.
     *
     * @see LauncherTasksModel
     * @see move
     * @see launchInPlace
     * @see separateLaunchers
     * @param separate Whether to keep launcher tasks separate.
     */
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
     * Returns whether grouping is done "inline" or not, i.e. whether groups
     * are maintained inside the flat, top-level list, or by forming a tree.
     * In inline grouping mode, move() on a group member will move all siblings
     * as well, and sorting is first done among groups, then group members.
     *
     * Further, in inline grouping mode, the groupingWindowTasksThreshold
     * setting is ignored: Grouping is always done.
     *
     * @see setGroupInline
     * @see move
     * @see groupingWindowTasksThreshold
     * @returns whether grouping is done inline or not.
     **/
    bool groupInline() const;

    /**
     * Sets whether grouping is done "inline" or not, i.e. whether groups
     * are maintained inside the flat, top-level list, or by forming a tree.
     * In inline grouping mode, move() on a group member will move all siblings
     * as well, and sorting is first done among groups, then group members.
     *
     * @see groupInline
     * @see move
     * @see groupingWindowTasksThreshold
     * @param inline Whether to do grouping inline or not.
     **/
    void setGroupInline(bool groupInline);

    /**
     * As window tasks (AbstractTasksModel::IsWindow) come and go, groups will
     * be formed when this threshold value is exceeded, and  broken apart when
     * it matches or falls below.
     *
     * Defaults to @c -1, which means grouping is done regardless of the number
     * of window tasks.
     *
     * When the groupInline property is set to @c true, the threshold is ignored:
     * Grouping is always done.
     *
     * @see setGroupingWindowTasksThreshold
     * @see groupInline
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
     * When the groupInline property is set to @c true, the threshold is ignored:
     * Grouping is always done.
     *
     * @see groupingWindowTasksThreshold
     * @see groupInline
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
     * Finds the first active (AbstractTasksModel::IsActive) task in the model
     * and returns its QModelIndex, or a null QModelIndex if no active task is
     * found.
     *
     * @returns the model index for the first active task, if any.
     */
    QModelIndex activeTask() const;

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
     * Request adding a launcher with the given URL to current activity.
     *
     * If this URL is already in the list, the request will fail. URLs are
     * compared for equality after removing the query string used to hold
     * metadata.
     *
     * @see launcherUrlsMatch
     * @param url A launcher URL.
     * @returns @c true if a launcher was added.
     */
    Q_INVOKABLE bool requestAddLauncherToActivity(const QUrl &url, const QString &activity);

    /**
     * Request removing the launcher with the given URL from the current activity.
     *
     * If this URL is already in the list, the request will fail. URLs are
     * compared for equality after removing the query string used to hold
     * metadata.
     *
     * @see launcherUrlsMatch
     * @param url A launcher URL.
     * @returns @c true if the launcher was removed.
     */
    Q_INVOKABLE bool requestRemoveLauncherFromActivity(const QUrl &url, const QString &activity);

    /**
     * Return the list of activities the launcher belongs to.
     * If there is no launcher with that url, the list will be empty,
     * while if the launcher is on all activities, it will contain a
     * null uuid.
     *
     * URLs are compared for equality after removing the query string used
     * to hold metadata.
     */
    Q_INVOKABLE QStringList launcherActivities(const QUrl &url);

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
    Q_INVOKABLE void requestActivate(const QModelIndex &index) override;

    /**
     * Request an additional instance of the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestNewInstance(const QModelIndex &index) override;

    /**
     * Requests to open the given URLs with the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     * @param urls The URLs to be passed to the application.
     **/
    Q_INVOKABLE void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) override;

    /**
     * Request the task at the given index be closed.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestClose(const QModelIndex &index) override;

    /**
     * Request starting an interactive move for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestMove(const QModelIndex &index) override;

    /**
     * Request starting an interactive resize for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be a
     * no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestResize(const QModelIndex &index) override;

    /**
     * Request toggling the minimized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleMinimized(const QModelIndex &index) override;

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleMaximized(const QModelIndex &index) override;

    /**
     * Request toggling the keep-above state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleKeepAbove(const QModelIndex &index) override;

    /**
     * Request toggling the keep-below state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleKeepBelow(const QModelIndex &index) override;

    /**
     * Request toggling the fullscreen state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleFullScreen(const QModelIndex &index) override;

    /**
     * Request toggling the shaded state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    Q_INVOKABLE void requestToggleShaded(const QModelIndex &index) override;

    /**
     * Request entering the window at the given index on the specified virtual desktops.
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
    Q_INVOKABLE void requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops) override;

    /**
     * Request entering the window at the given index on a new virtual desktop,
     * which is created in response to this request.
     *
     * @param index An index in this window tasks model.
     **/
    Q_INVOKABLE void requestNewVirtualDesktop(const QModelIndex &index) override;

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
    Q_INVOKABLE void requestActivities(const QModelIndex &index, const QStringList &activities) override;

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
        QObject *delegate = nullptr) override;

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
     * Moves a task to a new position in the list. The insert position is
     * is bounded to the list start and end.
     *
     * syncLaunchers() should be called after a set of move operations to
     * update the launcherList property to reflect the new order.
     *
     * When the groupInline property is set to @c true, a move request
     * for a group member will bring all siblings along.
     *
     * @see syncLaunchers
     * @see launcherList
     * @see setGroupInline
     * @param index An index in this tasks model.
     * @param newPos The new list position to move the task to.
     */
    Q_INVOKABLE bool move(int row, int newPos,
        const QModelIndex &parent = QModelIndex());

    /**
     * Updates the launcher list to reflect the new order after calls to
     * move(), if needed.
     *
     * @see move
     * @see launcherList
     */
    Q_INVOKABLE void syncLaunchers();

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

    /**
     * Given a row in the model, returns a QPersistentModelIndex for it. To get an index
     * for a child in a task group, an optional child row may be passed as well.
     *
     * @param row A row index in the model.
     * @param childRow A row index for a child of the task group at the given row.
     * @returns a model index for the task at the given row, or for one of its
     * child tasks.
     */
    Q_INVOKABLE QPersistentModelIndex makePersistentModelIndex(int row, int childRow = -1) const;

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void countChanged() const;
    void launcherCountChanged() const;
    void launcherListChanged() const;
    void anyTaskDemandsAttentionChanged() const;
    void virtualDesktopChanged() const;
    void screenGeometryChanged() const;
    void activityChanged() const;
    void filterByVirtualDesktopChanged() const;
    void filterByScreenChanged() const;
    void filterByActivityChanged() const;
    void filterNotMinimizedChanged() const;
    void sortModeChanged() const;
    void separateLaunchersChanged() const;
    void launchInPlaceChanged() const;
    void groupModeChanged() const;
    void groupInlineChanged() const;
    void groupingWindowTasksThresholdChanged() const;
    void groupingAppIdBlacklistChanged() const;
    void groupingLauncherUrlBlacklistChanged() const;
    void activeTaskChanged() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    Q_INVOKABLE void updateLauncherCount();

    class Private;
    class TasksModelLessThan;
    friend class TasksModelLessThan;
    QScopedPointer<Private> d;
};

}

#endif
