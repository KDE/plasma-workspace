/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "launchertasksmodel.h"
#include "tasktools.h"

#include <KDesktopFile>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <KSycoca>
#include <KWindowSystem>

#include <KActivities/Consumer>
#include <KActivities/ResourceInstance>

#include <KIO/ApplicationLauncherJob>

#include <QHash>
#include <QIcon>
#include <QSet>
#include <QTimer>
#include <QUrlQuery>

#include "launchertasksmodel_p.h"
#include <chrono>

using namespace std::chrono_literals;

namespace TaskManager
{
typedef QSet<QString> ActivitiesSet;

template<typename ActivitiesCollection>
inline bool isOnAllActivities(const ActivitiesCollection &activities)
{
    return activities.isEmpty() || activities.contains(NULL_UUID);
}

class Q_DECL_HIDDEN LauncherTasksModel::Private
{
public:
    Private(LauncherTasksModel *q);

    KActivities::Consumer activitiesConsumer;

    QList<QUrl> launchersOrder;

    QHash<QUrl, ActivitiesSet> activitiesForLauncher;
    inline void setActivitiesForLauncher(const QUrl &url, const ActivitiesSet &activities)
    {
        if (activities.size() == activitiesConsumer.activities().size()) {
            activitiesForLauncher[url] = {NULL_UUID};
        } else {
            activitiesForLauncher[url] = activities;
        }
    }

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
    sycocaChangeTimer.setInterval(100ms);

    QObject::connect(&sycocaChangeTimer, &QTimer::timeout, q, [this]() {
        if (!launchersOrder.count()) {
            return;
        }

        appDataCache.clear();

        // Emit changes of all roles satisfied from app data cache.
        Q_EMIT q->dataChanged(q->index(0, 0),
                              q->index(launchersOrder.count() - 1, 0),
                              QVector<int>{Qt::DisplayRole,
                                           Qt::DecorationRole,
                                           AbstractTasksModel::AppId,
                                           AbstractTasksModel::AppName,
                                           AbstractTasksModel::GenericName,
                                           AbstractTasksModel::LauncherUrl,
                                           AbstractTasksModel::LauncherUrlWithoutIcon});
    });

    QObject::connect(KSycoca::self(), &KSycoca::databaseChanged, q, [this]() {
        sycocaChangeTimer.start();
    });
}

AppData LauncherTasksModel::Private::appData(const QUrl &url)
{
    const auto &it = appDataCache.constFind(url);

    if (it != appDataCache.constEnd()) {
        return *it;
    }

    const AppData &data = appDataFromUrl(url, QIcon::fromTheme(QLatin1String("unknown")));

    appDataCache.insert(url, data);

    return data;
}

bool LauncherTasksModel::Private::requestAddLauncherToActivities(const QUrl &_url, const QStringList &_activities)
{
    QUrl url(_url);
    if (!isValidLauncherUrl(url)) {
        return false;
    }

    const auto activities = ActivitiesSet(_activities.cbegin(), _activities.cend());

    if (url.isLocalFile() && KDesktopFile::isDesktopFile(url.toLocalFile())) {
        KDesktopFile f(url.toLocalFile());

        const KService::Ptr service = KService::serviceByStorageId(f.fileName());

        // Resolve to non-absolute menuId-based URL if possible.
        if (service) {
            const QString &menuId = service->menuId();

            if (!menuId.isEmpty()) {
                url = QUrl(QLatin1String("applications:") + menuId);
            }
        }
    }

    // Merge duplicates
    int row = -1;
    foreach (const QUrl &launcher, launchersOrder) {
        ++row;

        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems)) {
            ActivitiesSet newActivities;

            // Use the key we established equivalence to ('launcher').
            if (!activitiesForLauncher.contains(launcher)) {
                // If we don't have the activities assigned to this url
                // for some reason
                newActivities = activities;

            } else {
                if (isOnAllActivities(activities)) {
                    // If the new list is empty, or has a null uuid, this
                    // launcher should be on all activities
                    newActivities = ActivitiesSet{NULL_UUID};

                } else if (isOnAllActivities(activitiesForLauncher[launcher])) {
                    // If we have been on all activities before, and we have
                    // been asked to be on a specific one, lets make an
                    // exception - we will set the activities to exactly
                    // what we have been asked
                    newActivities = activities;

                } else {
                    newActivities += activities;
                    newActivities += activitiesForLauncher[launcher];
                }
            }

            if (newActivities != activitiesForLauncher[launcher]) {
                setActivitiesForLauncher(launcher, newActivities);

                Q_EMIT q->dataChanged(q->index(row, 0), q->index(row, 0));

                Q_EMIT q->launcherListChanged();
                return true;
            }

            return false;
        }
    }

    // This is a new one
    const auto count = launchersOrder.count();
    q->beginInsertRows(QModelIndex(), count, count);
    setActivitiesForLauncher(url, activities);
    launchersOrder.append(url);
    q->endInsertRows();

    Q_EMIT q->launcherListChanged();

    return true;
}

bool LauncherTasksModel::Private::requestRemoveLauncherFromActivities(const QUrl &url, const QStringList &activities)
{
    for (int row = 0; row < launchersOrder.count(); ++row) {
        const QUrl launcher = launchersOrder.at(row);

        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems) || launcherUrlsMatch(url, appData(launcher).url, IgnoreQueryItems)) {
            const auto currentActivities = activitiesForLauncher[url];
            ActivitiesSet newActivities;

            bool remove = false;
            bool update = false;

            if (isOnAllActivities(currentActivities)) {
                // We are currently on all activities.
                // Should we go away, or just remove from the current one?

                if (isOnAllActivities(activities)) {
                    remove = true;

                } else {
                    const auto _activities = activitiesConsumer.activities();
                    for (const auto &activity : _activities) {
                        if (!activities.contains(activity)) {
                            newActivities << activity;
                        } else {
                            update = true;
                        }
                    }
                }

            } else if (isOnAllActivities(activities)) {
                remove = true;

            } else {
                // We weren't on all activities, just remove those that
                // we were on

                for (const auto &activity : currentActivities) {
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
                appDataCache.remove(launcher);
                launchersOrder.removeAt(row);
                activitiesForLauncher.remove(url);
                q->endRemoveRows();

            } else if (update) {
                setActivitiesForLauncher(url, newActivities);

                Q_EMIT q->dataChanged(q->index(row, 0), q->index(row, 0));
            }

            if (remove || update) {
                Q_EMIT q->launcherListChanged();
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
    } else if (role == IsVirtualDesktopsChangeable) {
        return false;
    } else if (role == IsOnAllVirtualDesktops) {
        return true;
    } else if (role == Activities) {
        return QStringList(d->activitiesForLauncher[url].values());
    } else if (role == CanLaunchNewInstance) {
        return false;
    }

    return QVariant();
}

int LauncherTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->launchersOrder.count();
}

int LauncherTasksModel::rowCountForActivity(const QString &activity) const
{
    if (activity == NULL_UUID || activity.isEmpty()) {
        return rowCount();
    }

    return std::count_if(d->launchersOrder.cbegin(), d->launchersOrder.cend(), [this, &activity](const QUrl &url) {
        const auto &set = d->activitiesForLauncher[url];
        return set.contains(NULL_UUID) || set.contains(activity);
    });
}

QStringList LauncherTasksModel::launcherList() const
{
    // Serializing the launchers
    QStringList result;

    for (const auto &launcher : qAsConst(d->launchersOrder)) {
        const auto &activities = d->activitiesForLauncher[launcher];

        QString serializedLauncher;
        if (isOnAllActivities(activities)) {
            serializedLauncher = launcher.toString();

        } else {
            serializedLauncher = "[" + d->activitiesForLauncher[launcher].values().join(",") + "]\n" + launcher.toString();
        }

        result << serializedLauncher;
    }

    return result;
}

void LauncherTasksModel::setLauncherList(const QStringList &serializedLaunchers)
{
    // Clearing everything
    QList<QUrl> newLaunchersOrder;
    QHash<QUrl, ActivitiesSet> newActivitiesForLauncher;

    // Loading the activity to launchers map
    for (const auto &serializedLauncher : serializedLaunchers) {
        QStringList _activities;
        QUrl url;

        std::tie(url, _activities) = deserializeLauncher(serializedLauncher);

        auto activities = ActivitiesSet(_activities.cbegin(), _activities.cend());

        // Is url is not valid, ignore it
        if (!isValidLauncherUrl(url)) {
            continue;
        }

        // If we have a null uuid, it means we are on all activities
        // and we should contain only the null uuid
        if (isOnAllActivities(activities)) {
            activities = {NULL_UUID};

        } else {
            // Filter out invalid activities
            const auto allActivities = d->activitiesConsumer.activities();
            ActivitiesSet validActivities;
            for (const auto &activity : qAsConst(activities)) {
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
        const auto location = std::find_if(newLaunchersOrder.begin(), newLaunchersOrder.end(), [&url](const QUrl &item) {
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

        } else if (newActivitiesForLauncher[url].contains(NULL_UUID)) {
            // Do nothing, we are already on all activities

        } else if (activities.contains(NULL_UUID)) {
            newActivitiesForLauncher[url] = {NULL_UUID};

        } else {
            // We are not on all activities, append the new ones
            newActivitiesForLauncher[url] += activities;
        }
    }

    if (newLaunchersOrder != d->launchersOrder) {
        const bool isOrderChanged = std::all_of(newLaunchersOrder.cbegin(),
                                                newLaunchersOrder.cend(),
                                                [this](const QUrl &url) {
                                                    return d->launchersOrder.contains(url);
                                                })
            && newLaunchersOrder.size() == d->launchersOrder.size();

        if (isOrderChanged) {
            for (int i = 0; i < newLaunchersOrder.size(); i++) {
                int oldRow = d->launchersOrder.indexOf(newLaunchersOrder.at(i));

                if (oldRow != i) {
                    beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), i);
                    d->launchersOrder.move(oldRow, i);
                    endMoveRows();
                }
            }
        } else {
            // Use Remove/Insert to update the manual sort map in TasksModel
            if (!d->launchersOrder.empty()) {
                beginRemoveRows(QModelIndex(), 0, d->launchersOrder.size() - 1);

                d->launchersOrder.clear();
                d->activitiesForLauncher.clear();

                endRemoveRows();
            }

            if (!newLaunchersOrder.empty()) {
                beginInsertRows(QModelIndex(), 0, newLaunchersOrder.size() - 1);

                d->launchersOrder = newLaunchersOrder;
                d->activitiesForLauncher = newActivitiesForLauncher;

                endInsertRows();
            }
        }

        Q_EMIT launcherListChanged();

    } else if (newActivitiesForLauncher != d->activitiesForLauncher) {
        for (int i = 0; i < d->launchersOrder.size(); i++) {
            const QUrl &url = d->launchersOrder.at(i);

            if (d->activitiesForLauncher[url] != newActivitiesForLauncher[url]) {
                d->activitiesForLauncher[url] = newActivitiesForLauncher[url];
                Q_EMIT dataChanged(index(i, 0), index(i, 0), {Activities});
            }
        }
    }
}

bool LauncherTasksModel::requestAddLauncher(const QUrl &url)
{
    return d->requestAddLauncherToActivities(url, {NULL_UUID});
}

bool LauncherTasksModel::requestRemoveLauncher(const QUrl &url)
{
    return d->requestRemoveLauncherFromActivities(url, {NULL_UUID});
}

bool LauncherTasksModel::requestAddLauncherToActivity(const QUrl &url, const QString &activity)
{
    return d->requestAddLauncherToActivities(url, {activity});
}

bool LauncherTasksModel::requestRemoveLauncherFromActivity(const QUrl &url, const QString &activity)
{
    return d->requestRemoveLauncherFromActivities(url, {activity});
}

QStringList LauncherTasksModel::launcherActivities(const QUrl &_url) const
{
    const auto position = launcherPosition(_url);

    if (position == -1) {
        // If we do not have this launcher, return an empty list
        return {};

    } else {
        const auto url = d->launchersOrder.at(position);

        // If the launcher is on all activities, return a null uuid
        return d->activitiesForLauncher.contains(url) ? d->activitiesForLauncher[url].values() : QStringList{NULL_UUID};
    }
}

int LauncherTasksModel::launcherPosition(const QUrl &url) const
{
    for (int i = 0; i < d->launchersOrder.count(); ++i) {
        if (launcherUrlsMatch(url, d->appData(d->launchersOrder.at(i)).url, IgnoreQueryItems)) {
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
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->launchersOrder.count()) {
        return;
    }

    runApp(d->appData(d->launchersOrder.at(index.row())));
}

void LauncherTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->launchersOrder.count() || urls.isEmpty()) {
        return;
    }

    const QUrl &url = d->launchersOrder.at(index.row());

    KService::Ptr service;

    if (url.scheme() == QLatin1String("applications")) {
        service = KService::serviceByMenuId(url.path());
    } else if (url.scheme() == QLatin1String("preferred")) {
        service = KService::serviceByStorageId(defaultApplication(url));
    } else {
        service = KService::serviceByDesktopPath(url.toLocalFile());
    }

    if (!service || !service->isApplication()) {
        return;
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->setUrls(urls);

    job->start();

    KActivities::ResourceInstance::notifyAccessed(QUrl(QStringLiteral("applications:") + service->storageId()), QStringLiteral("org.kde.libtaskmanager"));
}

}
