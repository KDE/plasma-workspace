/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "xstartuptasksmodel.h"

#include <KApplicationTrader>
#include <KConfig>
#include <KConfigGroup>
#include <KDirWatch>
#include <KService>
#include <KStartupInfo>

#include <QIcon>
#include <QTimer>
#include <QUrl>

namespace TaskManager
{
class Q_DECL_HIDDEN XStartupTasksModel::Private
{
public:
    Private(XStartupTasksModel *q);
    KDirWatch *configWatcher = nullptr;
    KStartupInfo *startupInfo = nullptr;
    QVector<KStartupInfoId> startups;
    QHash<QByteArray, KStartupInfoData> startupData;
    QHash<QByteArray, QUrl> launcherUrls;

    void init();
    void loadConfig();
    QUrl launcherUrl(const KStartupInfoData &data);

private:
    XStartupTasksModel *q;
};

XStartupTasksModel::Private::Private(XStartupTasksModel *q)
    : q(q)
{
}

void XStartupTasksModel::Private::init()
{
    configWatcher = new KDirWatch(q);
    configWatcher->addFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/klaunchrc"));

    QObject::connect(configWatcher, &KDirWatch::dirty, [this] {
        loadConfig();
    });
    QObject::connect(configWatcher, &KDirWatch::created, [this] {
        loadConfig();
    });
    QObject::connect(configWatcher, &KDirWatch::deleted, [this] {
        loadConfig();
    });

    loadConfig();
}

void XStartupTasksModel::Private::loadConfig()
{
    const KConfig _c("klaunchrc");
    KConfigGroup c(&_c, "FeedbackStyle");

    if (!c.readEntry("TaskbarButton", true)) {
        delete startupInfo;
        startupInfo = nullptr;

        q->beginResetModel();
        startups.clear();
        startupData.clear();
        q->endResetModel();

        return;
    }

    if (!startupInfo) {
        startupInfo = new KStartupInfo(KStartupInfo::CleanOnCantDetect, q);

        QObject::connect(startupInfo, &KStartupInfo::gotNewStartup, q, [this](const KStartupInfoId &id, const KStartupInfoData &data) {
            if (startups.contains(id)) {
                return;
            }

            const QString appId = data.applicationId();
            const QString bin = data.bin();

            foreach (const KStartupInfoData &known, startupData) {
                // Reject if we already have a startup notification for this app.
                if (known.applicationId() == appId && known.bin() == bin) {
                    return;
                }
            }

            const int count = startups.count();
            q->beginInsertRows(QModelIndex(), count, count);
            startups.append(id);
            startupData.insert(id.id(), data);
            launcherUrls.insert(id.id(), launcherUrl(data));
            q->endInsertRows();
        });

        QObject::connect(startupInfo, &KStartupInfo::gotRemoveStartup, q, [this](const KStartupInfoId &id) {
            // The order in which startups are cancelled and corresponding
            // windows appear is not reliable. Add some grace time to make
            // an overlap more likely, giving a proxy some time to arbitrate
            // between the two.
            QTimer::singleShot(500, q, [this, id]() {
                const int row = startups.indexOf(id);

                if (row != -1) {
                    q->beginRemoveRows(QModelIndex(), row, row);
                    startups.removeAt(row);
                    startupData.remove(id.id());
                    launcherUrls.remove(id.id());
                    q->endRemoveRows();
                }
            });
        });

        QObject::connect(startupInfo, &KStartupInfo::gotStartupChange, q, [this](const KStartupInfoId &id, const KStartupInfoData &data) {
            const int row = startups.indexOf(id);
            if (row != -1) {
                startupData.insert(id.id(), data);
                launcherUrls.insert(id.id(), launcherUrl(data));
                QModelIndex idx = q->index(row);
                Q_EMIT q->dataChanged(idx, idx);
            }
        });
    }

    c = KConfigGroup(&_c, "TaskbarButtonSettings");
    startupInfo->setTimeout(c.readEntry("Timeout", 5));
}

QUrl XStartupTasksModel::Private::launcherUrl(const KStartupInfoData &data)
{
    QUrl launcherUrl;
    KService::List services;

    QString appId = data.applicationId();

    // Try to match via desktop filename ...
    if (!appId.isEmpty() && appId.endsWith(QLatin1String(".desktop"))) {
        if (appId.startsWith(QLatin1String("/"))) {
            // Even if we have an absolute path, try resolving to a service first (Bug 385594)
            KService::Ptr service = KService::serviceByDesktopPath(appId);
            if (!service) { // No luck, just return it verbatim
                launcherUrl = QUrl::fromLocalFile(appId);
                return launcherUrl;
            }

            // Fall-through to menuId() handling below
            services = {service};
        } else {
            // turn into KService desktop entry name
            appId.chop(strlen(".desktop"));

            services = KApplicationTrader::query([&appId](const KService::Ptr &service) {
                return service->desktopEntryName().compare(appId, Qt::CaseInsensitive) == 0;
            });
        }
    }

    const QString wmClass = data.WMClass();

    // Try StartupWMClass.
    if (services.empty() && !wmClass.isEmpty()) {
        services = KApplicationTrader::query([&wmClass](const KService::Ptr &service) {
            return service->property(QStringLiteral("StartupWMClass")).toString().compare(wmClass, Qt::CaseInsensitive) == 0;
        });
    }

    const QString name = data.findName();

    // Try via name ...
    if (services.empty() && !name.isEmpty()) {
        services = KApplicationTrader::query([&name](const KService::Ptr &service) {
            return service->name().compare(name, Qt::CaseInsensitive) == 0;
        });
    }

    if (!services.empty()) {
        const QString &menuId = services.at(0)->menuId();

        // applications: URLs are used to refer to applications by their KService::menuId
        // (i.e. .desktop file name) rather than the absolute path to a .desktop file.
        if (!menuId.isEmpty()) {
            return QUrl(QStringLiteral("applications:") + menuId);
        }

        QString path = services.at(0)->entryPath();

        if (path.isEmpty()) {
            path = services.at(0)->exec();
        }

        if (!path.isEmpty()) {
            launcherUrl = QUrl::fromLocalFile(path);
        }
    }

    return launcherUrl;
}

XStartupTasksModel::XStartupTasksModel(QObject *parent)
    : AbstractTasksModel(parent)
    , d(new Private(this))
{
    d->init();
}

XStartupTasksModel::~XStartupTasksModel()
{
}

QVariant XStartupTasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= d->startups.count()) {
        return QVariant();
    }

    const QByteArray &id = d->startups.at(index.row()).id();

    if (!d->startupData.contains(id)) {
        return QVariant();
    }

    const KStartupInfoData &data = d->startupData.value(id);

    if (role == Qt::DisplayRole) {
        return data.findName();
    } else if (role == Qt::DecorationRole) {
        return QIcon::fromTheme(data.findIcon(), QIcon::fromTheme(QLatin1String("unknown")));
    } else if (role == AppId) {
        QString idFromPath = QUrl::fromLocalFile(data.applicationId()).fileName();

        if (idFromPath.endsWith(QLatin1String(".desktop"))) {
            idFromPath = idFromPath.left(idFromPath.length() - 8);
        }

        return idFromPath;
    } else if (role == AppName) {
        return data.findName();
    } else if (role == LauncherUrl || role == LauncherUrlWithoutIcon) {
        return d->launcherUrls.value(id);
    } else if (role == IsStartup) {
        return true;
    } else if (role == IsVirtualDesktopsChangeable) {
        return false;
    } else if (role == VirtualDesktops) {
        return QVariantList() << QVariant(data.desktop());
    } else if (role == IsOnAllVirtualDesktops) {
        return (data.desktop() == 0);
    } else if (role == CanLaunchNewInstance) {
        return false;
    } else if (role == BinaryName) {
        return data.bin();
    }

    return QVariant();
}

int XStartupTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->startups.count();
}

} // namespace TaskManager
