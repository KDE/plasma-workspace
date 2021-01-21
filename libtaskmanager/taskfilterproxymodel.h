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

#ifndef TASKFILTERPROXYMODEL_H
#define TASKFILTERPROXYMODEL_H

#include <QRect>
#include <QSortFilterProxyModel>

#include "abstracttasksproxymodeliface.h"

#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short A proxy tasks model filtering its source model by various properties.
 *
 * This proxy model class filters its source tasks model by properties such as
 * virtual desktop or minimixed state. The values to filter for or by are set
 * as properties on the proxy model instance.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT TaskFilterProxyModel : public QSortFilterProxyModel, public AbstractTasksProxyModelIface
{
    Q_OBJECT

    Q_PROPERTY(QVariant virtualDesktop READ virtualDesktop WRITE setVirtualDesktop NOTIFY virtualDesktopChanged)
    Q_PROPERTY(QRect screenGeometry READ screenGeometry WRITE setScreenGeometry NOTIFY screenGeometryChanged)
    Q_PROPERTY(QString activity READ activity WRITE setActivity NOTIFY activityChanged)

    Q_PROPERTY(bool filterByVirtualDesktop READ filterByVirtualDesktop WRITE setFilterByVirtualDesktop NOTIFY filterByVirtualDesktopChanged)
    Q_PROPERTY(bool filterByScreen READ filterByScreen WRITE setFilterByScreen NOTIFY filterByScreenChanged)
    Q_PROPERTY(bool filterByActivity READ filterByActivity WRITE setFilterByActivity NOTIFY filterByActivityChanged)
    Q_PROPERTY(bool filterNotMinimized READ filterNotMinimized WRITE setFilterNotMinimized NOTIFY filterNotMinimizedChanged)
    Q_PROPERTY(bool filterSkipTaskbar READ filterSkipTaskbar WRITE setFilterSkipTaskbar NOTIFY filterSkipTaskbarChanged)
    Q_PROPERTY(bool filterSkipPager READ filterSkipPager WRITE setFilterSkipPager NOTIFY filterSkipPagerChanged)

    Q_PROPERTY(bool demandingAttentionSkipsFilters READ demandingAttentionSkipsFilters WRITE setDemandingAttentionSkipsFilters NOTIFY
                   demandingAttentionSkipsFiltersChanged)

public:
    explicit TaskFilterProxyModel(QObject *parent = nullptr);
    ~TaskFilterProxyModel() override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

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
     * number greater than -1 is set, even if this returns @c true.
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
     * Whether tasks which should be omitted from 'task bars' should be
     * filtered. Defaults to @c true.
     *
     * @see setFilterSkipTaskbar
     * @returns @c true if tasks which should not be on the 'task bar'
     * should be filtered.
     **/
    bool filterSkipTaskbar() const;

    /**
     * Set whether tasks which should be omitted from 'task bars' should be
     * filtered.
     *
     * @see filterSkipTaskbar
     * @param filter Whether tasks whichs should not be on the 'task bar'
     * should be filtered.
     **/
    void setFilterSkipTaskbar(bool filter);

    /**
     * Whether tasks which should be omitted from 'pagers' should be
     * filtered. Defaults to @c true.
     *
     * @see setFilterSkipPager
     * @returns @c true if tasks which should not be on the 'pager' should
     * be filtered.
     **/
    bool filterSkipPager() const;

    /**
     * Set whether tasks which should be omitted from 'pagers' should be
     * filtered.
     *
     * @see filterSkipPager
     * @param filter Whether tasks which should not be on the 'pager' should
     * be filtered.
     **/
    void setFilterSkipPager(bool filter);

    /**
     * Whether tasks which demand attention skip filters by virtual desktop
     * or activity. Defaults to @c true.
     *
     * @see setDemandingAttentionSkipsFilters
     * @returns @c true if tasks which demand attention skip filters.
     **/
    bool demandingAttentionSkipsFilters() const;

    /**
     * Sets whether tasks which demand attention should bypass filters by
     * virtual desktop or activity.
     *
     * @see demandingAttentionSkipsFilters
     * @param skip Whether tasks which demand attention should skip filters.
     **/
    void setDemandingAttentionSkipsFilters(bool skip);

    /**
     * Returns whether the filter model accepts this source row.
     *
     * @param int A row in the source model.
     */
    bool acceptsRow(int sourceRow) const;

Q_SIGNALS:
    void virtualDesktopChanged() const;
    void screenGeometryChanged() const;
    void activityChanged() const;
    void filterByVirtualDesktopChanged() const;
    void filterByScreenChanged() const;
    void filterByActivityChanged() const;
    void filterNotMinimizedChanged() const;
    void filterSkipTaskbarChanged() const;
    void filterSkipPagerChanged() const;
    void demandingAttentionSkipsFiltersChanged() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    QModelIndex mapIfaceToSource(const QModelIndex &index) const override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
