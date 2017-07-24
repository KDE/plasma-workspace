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

#include <config-X11.h>

#include <QIcon>
#include <QTimer>
#if HAVE_X11
#include <QX11Info>
#endif

namespace TaskManager
{

class LauncherTasksModel::Private
{
public:
    Private(LauncherTasksModel *q);
    QList<QUrl> launchers;
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
            if (!launchers.count()) {
                return;
            }

            appDataCache.clear();

            // Emit changes of all roles satisfied from app data cache.
            q->dataChanged(q->index(0, 0),  q->index(launchers.count() - 1, 0),
                QVector<int>{Qt::DisplayRole, Qt::DecorationRole,
                AbstractTasksModel::AppId, AbstractTasksModel::AppName,
                AbstractTasksModel::GenericName, AbstractTasksModel::LauncherUrl,
                AbstractTasksModel::LauncherUrlWithoutIcon});
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
    if (!index.isValid() || index.row() >= d->launchers.count()) {
        return QVariant();
    }

    const QUrl &url = d->launchers.at(index.row());
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
    }

    return QVariant();
}

int LauncherTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->launchers.count();
}

QStringList LauncherTasksModel::launcherList() const
{
    return QUrl::toStringList(d->launchers);
}

void LauncherTasksModel::setLauncherList(const QStringList &launchers)
{
    const QList<QUrl> &_urls = QUrl::fromStringList(launchers, QUrl::StrictMode);
    QList<QUrl> urls;

    // Reject invalid urls and duplicates.
    foreach(const QUrl &url, _urls) {
        if (url.isValid()) {
            bool dupe = false;

            foreach(const QUrl &addedUrl, urls) {
                dupe = launcherUrlsMatch(url, addedUrl, IgnoreQueryItems);
                if (dupe) break;
            }

            if (!dupe) {
                urls.append(url);
            }
        }
    }

    if (d->launchers != urls) {
        // Common case optimization: If the list changed but its size
        // did not (e.g. due to reordering by a user of this model),
        // just clear the caches and announce new data instead of
        // resetting.
        if (d->launchers.count() == urls.count()) {
            d->launchers.clear();
            d->appDataCache.clear();

            d->launchers = urls;

            emit dataChanged(index(0, 0), index(d->launchers.count() - 1, 0));
        } else {
            beginResetModel();

            d->launchers.clear();
            d->appDataCache.clear();

            d->launchers = urls;

            endResetModel();
        }

        emit launcherListChanged();
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
    foreach(const QUrl &launcher, d->launchers) {
        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems)) {
            return false;
        }
    }

    const int count = d->launchers.count();
    beginInsertRows(QModelIndex(), count, count);
    d->launchers.append(url);
    endInsertRows();

    emit launcherListChanged();

    return true;
}

bool LauncherTasksModel::requestRemoveLauncher(const QUrl &url)
{
    for (int i = 0; i < d->launchers.count(); ++i) {
        const QUrl &launcher = d->launchers.at(i);

        if (launcherUrlsMatch(url, launcher, IgnoreQueryItems)
            || launcherUrlsMatch(url, d->appData(launcher).url, IgnoreQueryItems)) {
            beginRemoveRows(QModelIndex(), i, i);
            d->launchers.removeAt(i);
            d->appDataCache.remove(launcher);
            endRemoveRows();

            emit launcherListChanged();

            return true;
        }
    }

    return false;
}

int LauncherTasksModel::launcherPosition(const QUrl &url) const
{
    for (int i = 0; i < d->launchers.count(); ++i) {
        if (launcherUrlsMatch(url, d->launchers.at(i), IgnoreQueryItems)) {
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
        || index.row() < 0 || index.row() >= d->launchers.count()) {
        return;
    }

    const QUrl &url = d->launchers.at(index.row());

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
        || index.row() < 0 || index.row() >= d->launchers.count()
        || urls.isEmpty()) {
        return;
    }

    const QUrl &url = d->launchers.at(index.row());

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
