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

    QList<QUrl> currentlyShownLaunchers;
    QList<QUrl> launchersOrder;
    QHash<QUrl, QStringList> activitiesForLauncher;

    inline QList<QUrl> launchersForActivity(const QString &activity) const
    {
        QList<QUrl> result;
        qDebug() << "GREPME: launchers" << launchersOrder;
        for (const auto &launcher: launchersOrder) {
            const auto activities = activitiesForLauncher[launcher];
            qDebug() << "GREPME: activities for launcher" << launcher << activities;
            if (activities.contains(NULL_UUID) || activities.contains(activity)) {
                result << launcher;
            }
        }
        return result;
    }

    inline void updateShownLaunchers()
    {
        const auto urls = launchersForActivity(activities.currentActivity());

        qDebug() << "GREPME: Urls for this activity are: " << urls;

        if (currentlyShownLaunchers != urls) {
            // Common case optimization: If the list changed but its size
            // did not (e.g. due to reordering by a user of this model),
            // just clear the caches and announce new data instead of
            // resetting.
            if (currentlyShownLaunchers.count() == urls.count()) {
                currentlyShownLaunchers.clear();
                appDataCache.clear();

                currentlyShownLaunchers = urls;

                emit q->dataChanged(
                        q->index(0, 0),
                        q->index(currentlyShownLaunchers.count() - 1, 0));
            } else {
                q->beginResetModel();

                currentlyShownLaunchers.clear();
                appDataCache.clear();

                currentlyShownLaunchers = urls;

                q->endResetModel();

                emit q->shownLauncherListChanged();
            }
        }
    }

    QHash<QUrl, AppData> appDataCache;
    QTimer sycocaChangeTimer;

    void init();
    AppData appData(const QUrl &url);

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
            if (!currentlyShownLaunchers.count()) {
                return;
            }

            appDataCache.clear();

            // Emit changes of all roles satisfied from app data cache.
            q->dataChanged(q->index(0, 0),  q->index(currentlyShownLaunchers.count() - 1, 0),
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

    QObject::connect(&activities, &KActivities::Consumer::currentActivityChanged, q,
        [this]() {
            updateShownLaunchers();
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
    if (!index.isValid() || index.row() >= d->currentlyShownLaunchers.count()) {
        qDebug() << "GREPME NOT A VALID INDEX" << index.row();
        return QVariant();
    }

    const QUrl &url = d->currentlyShownLaunchers.at(index.row());
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
    return parent.isValid() ? 0 : d->currentlyShownLaunchers.count();
}

QStringList LauncherTasksModel::shownLauncherList() const
{
    return QUrl::toStringList(d->currentlyShownLaunchers);
}

QStringList LauncherTasksModel::serializedLauncherList() const
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

void LauncherTasksModel::setSerializedLauncherList(const QStringList &serializedLaunchers)
{
    // Clearing everything
    d->launchersOrder.clear();

    QHash<QUrl, QStringList> oldActivitiesForLauncher;
    std::swap(oldActivitiesForLauncher, d->activitiesForLauncher);

    qDebug() << "GREPME: We are asked to set these launchers:"
             << serializedLaunchers
        ;

    // Loading the activity to launchers map
    QHash<QString, QList<QUrl>> launchersForActivitiesCandidates;
    for (const auto& serializedLauncher: serializedLaunchers) {
        QStringList activities;
        QUrl url;

        std::tie(url, activities) =
            deserializeLauncher(serializedLauncher);

        qDebug() << "GREPME: Result: " << url << activities;

        // Is url is not valid, ignore it
        if (!url.isValid()) continue;

        qDebug() << "GREPME: Url is valid";

        // Is the url a duplicate?
        const auto location =
            std::find_if(d->launchersOrder.begin(), d->launchersOrder.end(),
                [&url] (const QUrl &item) {
                    return launcherUrlsMatch(url, item, IgnoreQueryItems);
                });


        if (location != d->launchersOrder.end()) {
            // It is a duplicate
            url = *location;
        } else {
            // It is not a duplicate, we need to add it
            // to the list of registered launchers
            d->launchersOrder << url;
        }

        d->activitiesForLauncher[url].append(activities);

        // If this is shown on all activities, we do not need to remember
        // each activity separately
        if (d->activitiesForLauncher[url].contains(NULL_UUID)) {
            d->activitiesForLauncher[url] = QStringList({ NULL_UUID });
        }
    }

    qDebug() << "GREPME: We got:" << d->activitiesForLauncher;
    qDebug() << "GREPME: We got order:" << d->launchersOrder;

    if (oldActivitiesForLauncher != d->activitiesForLauncher) {
        d->updateShownLaunchers();
        emit serializedLauncherListChanged();
    }
}

bool LauncherTasksModel::requestAddLauncher(const QUrl &_url)
{
    // isValid() for the passed-in URL might return true if it was
    // constructed in TolerantMode, but we want to reject invalid URLs.
    QUrl url(_url.toString(), QUrl::StrictMode);

    if (url.isEmpty() || !url.isValid()) {
        return false;
    }

    // Reject duplicates.
    foreach(const QUrl &launcher, d->launchersOrder) {
        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems)) {
            return false;
        }
    }

    // Adding the launcher to all activities
    const int count = d->launchersOrder.count();
    beginInsertRows(QModelIndex(), count, count);
    d->activitiesForLauncher[url] = QStringList({ NULL_UUID });
    d->launchersOrder.append(url);
    endInsertRows();

    emit serializedLauncherListChanged();
    emit shownLauncherListChanged();

    return true;
}

bool LauncherTasksModel::requestRemoveLauncher(const QUrl &url)
{
    for (int i = 0; i < d->launchersOrder.count(); ++i) {
        const QUrl &launcher = d->launchersOrder.at(i);

        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems)
            || launcherUrlsMatch(url, d->appData(launcher).url, IgnoreQueryItems)) {

            // Removing the launcher from all activities

            beginRemoveRows(QModelIndex(), i, i);
            d->launchersOrder.removeAt(i);
            d->activitiesForLauncher.remove(url);
            d->appDataCache.remove(launcher);
            endRemoveRows();

            emit serializedLauncherListChanged();
            emit shownLauncherListChanged();

            return true;
        }
    }

    return false;
}

int LauncherTasksModel::launcherPosition(const QUrl &url) const
{
    for (int i = 0; i < d->currentlyShownLaunchers.count(); ++i) {
        if (launcherUrlsMatch(url, d->currentlyShownLaunchers.at(i), IgnoreQueryItems)) {
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
        || index.row() < 0 || index.row() >= d->currentlyShownLaunchers.count()) {
        return;
    }

    const QUrl &url = d->currentlyShownLaunchers.at(index.row());

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
        || index.row() < 0 || index.row() >= d->currentlyShownLaunchers.count()
        || urls.isEmpty()) {
        return;
    }

    const QUrl &url = d->currentlyShownLaunchers.at(index.row());

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
