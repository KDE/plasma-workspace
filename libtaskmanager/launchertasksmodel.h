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

#ifndef LAUNCHERTASKSMODEL_H
#define LAUNCHERTASKSMODEL_H

#include "abstracttasksmodel.h"

#include "taskmanager_export.h"

#include <QUrl>

namespace TaskManager
{
/**
 * @short A tasks model for launchers.
 *
 * This model presents tasks sourced from list of launcher URLs. The
 * list can be read from and written to from a notifiable prop, enabling
 * storage outside the instance (e.g. in config).
 *
 * Extends AbstractTasksModel API with API for adding, removing, checking
 * for and moving launchers by URL or row index.
 *
 * Launcher URLs can use the preferred:// protocol to request system
 * default applications such as "browser" and "mailer".
 *
 * @see defaultApplication
 *
 * @author Eike Hein <hein@kde.org>
 */

class TASKMANAGER_EXPORT LauncherTasksModel : public AbstractTasksModel
{
    Q_OBJECT

    Q_PROPERTY(QStringList launcherList READ launcherList WRITE setLauncherList NOTIFY launcherListChanged)

public:
    explicit LauncherTasksModel(QObject *parent = nullptr);
    ~LauncherTasksModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

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
    bool requestAddLauncher(const QUrl &url);

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
    bool requestRemoveLauncher(const QUrl &url);

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
    bool requestAddLauncherToActivity(const QUrl &url, const QString &activity);

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
    bool requestRemoveLauncherFromActivity(const QUrl &url, const QString &activity);

    /**
     * Return the list of activities the launcher belongs to.
     * If there is no launcher with that url, the list will be empty,
     * while if the launcher is on all activities, it will contain a
     * null uuid.
     *
     * URLs are compared for equality after removing the query string used
     * to hold metadata.
     */
    QStringList launcherActivities(const QUrl &url) const;

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
    int launcherPosition(const QUrl &url) const;

    /**
     * Runs the URL (i.e. launches the application) at the given index.
     *
     * @param index An index in this launcher tasks model.
     */
    void requestActivate(const QModelIndex &index) override;

    /**
     * Runs the URL (i.e. launches the application) at the given index.
     *
     * @param index An index in this launcher tasks model.
     */
    void requestNewInstance(const QModelIndex &index) override;

    /**
     * Runs the application backing the launcher at the given index with the given URLs.
     *
     * @param index An index in this launcher tasks model
     * @param urls The URLs to be passed to the application
     */
    void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) override;

Q_SIGNALS:
    void launcherListChanged() const;

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
