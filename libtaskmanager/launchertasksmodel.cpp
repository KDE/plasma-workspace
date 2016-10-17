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

#include "launchertasksmodel.h"
#include "tasktools.h"

#include <KRun>
#include <KService>
#include <KStartupInfo>
#include <KSycoca>
#include <KWindowSystem>

#include <KActivities/Consumer>

#include <config-X11.h>

#include <QIcon>
#include <QTimer>
#if HAVE_X11
#include <QX11Info>
#endif

#include "launchertasksmodel_p.h"

namespace TaskManager
{

class LauncherTasksModel::Private
{
public:
    Private(LauncherTasksModel *q);

    KActivities::Consumer activities;

    QList<QUrl> launchersOrder;
    QHash<QUrl, QStringList> activitiesForLauncher;

    QHash<QUrl, AppData> appDataCache;
    QTimer sycocaChangeTimer;

    void init();
    AppData appData(const QUrl &url);

    bool requestAddLauncherToActivities(const QUrl &_url, const QStringList &activities);
    bool requestRemoveLauncherFromActivities(const QUrl &_url, const QStringList &activities);

private:
    LauncherTasksModel *q;
};

LauncherTasksModel::Private::Private(LauncherTasksModel *q)
    : q(q)
{
}

void LauncherTasksModel::Private::init()
{
    sycocaChangeTimer.setSingleShot(true);
    sycocaChangeTimer.setInterval(100);

    QObject::connect(&sycocaChangeTimer, &QTimer::timeout, q,
        [this]() {
            if (!launchersOrder.count()) {
                return;
            }

            appDataCache.clear();

            // Emit changes of all roles satisfied from app data cache.
            q->dataChanged(q->index(0, 0),  q->index(launchersOrder.count() - 1, 0),
                QVector<int>{Qt::DisplayRole, Qt::DecorationRole,
                AbstractTasksModel::AppId, AbstractTasksModel::AppName,
                AbstractTasksModel::GenericName, AbstractTasksModel::LauncherUrl});
        }
    );

    void (KSycoca::*myDatabaseChangeSignal)(const QStringList &) = &KSycoca::databaseChanged;
    QObject::connect(KSycoca::self(), myDatabaseChangeSignal, q,
        [this](const QStringList &changedResources) {
            if (changedResources.contains(QLatin1String("services"))
                || changedResources.contains(QLatin1String("apps"))
                || changedResources.contains(QLatin1String("xdgdata-apps"))) {
                sycocaChangeTimer.start();
            }
        }
    );
}

AppData LauncherTasksModel::Private::appData(const QUrl &url)
{
    if (!appDataCache.contains(url)) {
        const AppData &data = appDataFromUrl(url, QIcon::fromTheme(QLatin1String("unknown")));
        appDataCache.insert(url, data);

        return data;
    }

    return appDataCache.value(url);
}

bool LauncherTasksModel::Private::requestAddLauncherToActivities(const QUrl &_url, const QStringList &activities)
{
    // isValid() for the passed-in URL might return true if it was
    // constructed in TolerantMode, but we want to reject invalid URLs.
    QUrl url(_url.toString(), QUrl::StrictMode);

    if (url.isEmpty() || !url.isValid()) {
        return false;
    }

    // Merge duplicates
    int row = -1;
    foreach(const QUrl &launcher, launchersOrder) {
        ++row;

        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems)) {
            QStringList newActivities;

            if (!activitiesForLauncher.contains(url)) {
                // If we don't have the activities assigned to this url
                // for some reason
                newActivities = activities;

            } else {
                // If any of the lists are empty, we are on all activities,
                // otherwise, lets merge the lists
                if (activitiesForLauncher[url].isEmpty() || activities.isEmpty()) {
                    newActivities.clear();

                } else {
                    newActivities.append(activities);
                    newActivities.append(activitiesForLauncher[url]);

                }
            }

            if (newActivities != activitiesForLauncher[url]) {
                emit q->dataChanged(
                        q->index(row, 0),
                        q->index(row, 0));
                return true;

            }

            return false;
        }
    }

    // This is a new one
    const auto count = launchersOrder.count();
    q->beginInsertRows(QModelIndex(), count, count);
    activitiesForLauncher[url] = activities;
    launchersOrder.append(url);
    q->endInsertRows();

    emit q->launcherListChanged();

    return true;
}

bool LauncherTasksModel::Private::requestRemoveLauncherFromActivities(const QUrl &url, const QStringList &activities)
{
    for (int row = 0; row < launchersOrder.count(); ++row) {
        const QUrl &launcher = launchersOrder.at(row);

        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems)
            || launcherUrlsMatch(url, appData(launcher).url, IgnoreQueryItems)) {

            const QStringList currentActivities = activitiesForLauncher[url];
            QStringList newActivities;
            bool remove = false;
            bool update = false;

            if (currentActivities.isEmpty()) {
                // We are currently on all activities.
                // Should we go away, or just remove from the current one?
                if (activities.isEmpty()) {
                    remove = true;

                } else {
                    for (const auto& activity: currentActivities) {
                        if (!activities.contains(activity)) {
                            newActivities << activity;
                        } else {
                            update = true;
                        }
                    }
                }

            } else {
                // We weren't on all activities, just remove those that
                // we were on

                for (const auto& activity: currentActivities) {
                    if (!activities.contains(activity)) {
                        newActivities << activity;
                    }
                }

                if (newActivities.isEmpty()) {
                    remove = true;
                } else {
                    update = true;
                }
            }

            if (remove) {
                q->beginRemoveRows(QModelIndex(), row, row);
                launchersOrder.removeAt(row);
                activitiesForLauncher.remove(url);
                appDataCache.remove(launcher);
                q->endRemoveRows();

            } else if (update) {
                activitiesForLauncher[url] = newActivities;

                emit q->dataChanged(
                        q->index(row, 0),
                        q->index(row, 0));
            }

            if (remove || update) {
                emit q->launcherListChanged();
                return true;
            }
        }
    }

    return false;
}

LauncherTasksModel::LauncherTasksModel(QObject *parent)
    : AbstractTasksModel(parent)
    , d(new Private(this))
{
    d->init();
}

LauncherTasksModel::~LauncherTasksModel()
{
}

QVariant LauncherTasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= d->launchersOrder.count()) {
        return QVariant();
    }

    const QUrl &url = d->launchersOrder.at(index.row());
    const AppData &data = d->appData(url);
    if (role == Qt::DisplayRole) {
        return data.name;
    } else if (role == Qt::DecorationRole) {
        return data.icon;
    } else if (role == AppId) {
        return data.id;
    } else if (role == AppName) {
        return data.name;
    } else if (role == GenericName) {
        return data.genericName;
    } else if (role == LauncherUrl) {
        // Take resolved URL from cache.
        return data.url;
    } else if (role == LauncherUrlWithoutIcon) {
        // Take resolved URL from cache.
        QUrl url = data.url;

        if (url.hasQuery()) {
            QUrlQuery query(url);
            query.removeQueryItem(QLatin1String("iconData"));
            url.setQuery(query);
        }

        return url;
    } else if (role == IsLauncher) {
        return true;
    } else if (role == IsOnAllVirtualDesktops) {
        return true;
    } else if (role == Activities) {
        return d->activitiesForLauncher[url];
    }

    return QVariant();
}

int LauncherTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->launchersOrder.count();
}

QStringList LauncherTasksModel::launcherList() const
{
    // Serializing the launchers
    QStringList result;

    for (const auto &launcher: d->launchersOrder) {
        const auto &activities = d->activitiesForLauncher[launcher];

        QString serializedLauncher;
        if (activities.isEmpty()) {
            serializedLauncher = launcher.toString();

        } else {
            serializedLauncher =
                "[" + d->activitiesForLauncher[launcher].join(",") + "]\n" +
                launcher.toString();
        }

        result << serializedLauncher;
    }

    return result;
}

void LauncherTasksModel::setLauncherList(const QStringList &serializedLaunchers)
{
    // Clearing everything
    QList<QUrl> newLaunchersOrder;
    QHash<QUrl, QStringList> newActivitiesForLauncher;

    // Loading the activity to launchers map
    QHash<QString, QList<QUrl>> launchersForActivitiesCandidates;
    for (const auto& serializedLauncher: serializedLaunchers) {
        QStringList activities;
        QUrl url;

        std::tie(url, activities) =
            deserializeLauncher(serializedLauncher);

        // Is url is not valid, ignore it
        if (!url.isValid()) continue;

        // If we have a null uuid, it means we are on all activities
        if (activities.contains(NULL_UUID)) {
            activities.clear();
        }

        // Filter invalid activities
        if (!activities.isEmpty()) {
            const auto allActivities = d->activities.activities();
            QStringList validActivities;
            for (const auto& activity: activities) {
                if (allActivities.contains(activity)) {
                    validActivities << activity;
                }
            }

            if (validActivities.isEmpty()) {
                // If all activities that had this launcher are
                // removed, we are killing the launcher as well
                continue;
            }

            activities = validActivities;
        }

        // Is the url a duplicate?
        const auto location =
            std::find_if(newLaunchersOrder.begin(), newLaunchersOrder.end(),
                [&url] (const QUrl &item) {
                    return launcherUrlsMatch(url, item, IgnoreQueryItems);
                });


        if (location != newLaunchersOrder.end()) {
            // It is a duplicate
            url = *location;

        } else {
            // It is not a duplicate, we need to add it
            // to the list of registered launchers
            newLaunchersOrder << url;
        }


        if (!newActivitiesForLauncher.contains(url)) {
            // This is the first time we got this url
            newActivitiesForLauncher[url] = activities;

        } else if (newActivitiesForLauncher[url].isEmpty()) {
            // Do nothing, we are already on all activities

        } else {
            // We are not on all activities, append the new ones
            newActivitiesForLauncher[url].append(activities);

        }
    }

    if (newActivitiesForLauncher != d->activitiesForLauncher) {
        // Common case optimization: If the list changed but its size
        // did not (e.g. due to reordering by a user of this model),
        // just clear the caches and announce new data instead of
        // resetting.
        if (newLaunchersOrder.count() == d->launchersOrder.count()) {
            d->appDataCache.clear();

            std::swap(newLaunchersOrder, d->launchersOrder);
            std::swap(newActivitiesForLauncher, d->activitiesForLauncher);

            emit dataChanged(
                    index(0, 0),
                    index(d->launchersOrder.count() - 1, 0));

        } else {
            beginResetModel();

            std::swap(newLaunchersOrder, d->launchersOrder);
            std::swap(newActivitiesForLauncher, d->activitiesForLauncher);

            d->appDataCache.clear();

            endResetModel();

        }

        emit launcherListChanged();
    }
}

bool LauncherTasksModel::requestAddLauncher(const QUrl &url)
{
    return d->requestAddLauncherToActivities(url, QStringList());
}

bool LauncherTasksModel::requestRemoveLauncher(const QUrl &url)
{
    return d->requestRemoveLauncherFromActivities(url, QStringList());
}

bool LauncherTasksModel::requestAddLauncherToActivity(const QUrl &url)
{
    return d->requestAddLauncherToActivities(url, { d->activities.currentActivity() });
}

bool LauncherTasksModel::requestRemoveLauncherFromActivity(const QUrl &url)
{
    return d->requestRemoveLauncherFromActivities(url, { d->activities.currentActivity() });
}

QStringList LauncherTasksModel::launcherActivities(const QUrl &_url) const
{
    const auto position = launcherPosition(_url);
    const auto url = d->launchersOrder.at(position);

    return d->activitiesForLauncher.contains(url) ? d->activitiesForLauncher[url]
                                                  : QStringList { NULL_UUID };
}

int LauncherTasksModel::launcherPosition(const QUrl &url) const
{
    for (int i = 0; i < d->launchersOrder.count(); ++i) {
        if (launcherUrlsMatch(url, d->launchersOrder.at(i), IgnoreQueryItems)) {
            return i;
        }
    }

    return -1;
}

void LauncherTasksModel::requestActivate(const QModelIndex &index)
{
    requestNewInstance(index);
}

void LauncherTasksModel::requestNewInstance(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this
        || index.row() < 0 || index.row() >= d->launchersOrder.count()) {
        return;
    }

    const QUrl &url = d->launchersOrder.at(index.row());

    quint32 timeStamp = 0;

#if HAVE_X11
        if (KWindowSystem::isPlatformX11()) {
            timeStamp = QX11Info::appUserTime();
        }
#endif

    if (url.scheme() == QLatin1String("preferred")) {
        KService::Ptr service = KService::serviceByStorageId(defaultApplication(url));

        if (!service) {
            return;
        }

        new KRun(QUrl::fromLocalFile(service->entryPath()), 0, false,
            KStartupInfo::createNewStartupIdForTimestamp(timeStamp));
    } else {
        new KRun(url, 0, false, KStartupInfo::createNewStartupIdForTimestamp(timeStamp));
    }
}

void LauncherTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!index.isValid() || index.model() != this
        || index.row() < 0 || index.row() >= d->launchersOrder.count()
        || urls.isEmpty()) {
        return;
    }

    const QUrl &url = d->launchersOrder.at(index.row());

    quint32 timeStamp = 0;

#if HAVE_X11
        if (KWindowSystem::isPlatformX11()) {
            timeStamp = QX11Info::appUserTime();
        }
#endif

    KService::Ptr service;

    if (url.scheme() == QLatin1String("preferred")) {
        service = KService::serviceByStorageId(defaultApplication(url));
    } else {
        service = KService::serviceByDesktopPath(url.toLocalFile());
    }

    if (!service) {
        return;
    }

    KRun::runApplication(*service, urls, nullptr, 0, {}, KStartupInfo::createNewStartupIdForTimestamp(timeStamp));
}

}
