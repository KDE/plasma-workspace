/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recentusagemodel.h"
#include "actionlist.h"
#include "appentry.h"
#include "appsmodel.h"
#include "debug.h"
#include "kastatsfavoritesmodel.h"
#include <kio_version.h>

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QMimeDatabase>
#include <QQmlEngine>
#include <QTimer>

#include <KActivities/ResourceInstance>
#include <KFileItem>
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KService/KApplicationTrader>
#include <KService>

#include <KActivities/Stats/Cleaning>
#include <KActivities/Stats/Terms>
#include <KWindowSystem>

namespace KAStats = KActivities::Stats;

using namespace KAStats;
using namespace KAStats::Terms;

GroupSortProxy::GroupSortProxy(AbstractModel *parentModel, QAbstractItemModel *sourceModel)
    : QSortFilterProxyModel(parentModel)
{
    sourceModel->setParent(this);
    setSourceModel(sourceModel);
    sort(0);
}

GroupSortProxy::~GroupSortProxy()
{
}

InvalidAppsFilterProxy::InvalidAppsFilterProxy(AbstractModel *parentModel, QAbstractItemModel *sourceModel)
    : QSortFilterProxyModel(parentModel)
    , m_parentModel(parentModel)
{
    connect(parentModel, &AbstractModel::favoritesModelChanged, this, &InvalidAppsFilterProxy::connectNewFavoritesModel);
    connectNewFavoritesModel();

    sourceModel->setParent(this);
    setSourceModel(sourceModel);
}

InvalidAppsFilterProxy::~InvalidAppsFilterProxy()
{
}

void InvalidAppsFilterProxy::connectNewFavoritesModel()
{
    KAStatsFavoritesModel *favoritesModel = static_cast<KAStatsFavoritesModel *>(m_parentModel->favoritesModel());
    if (favoritesModel) {
        connect(favoritesModel, &KAStatsFavoritesModel::favoritesChanged, this, &QSortFilterProxyModel::invalidate);
    }

    invalidate();
}

bool InvalidAppsFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent);

    const QString resource = sourceModel()->index(source_row, 0).data(ResultModel::ResourceRole).toString();

    if (resource.startsWith(QLatin1String("applications:"))) {
        KService::Ptr service = KService::serviceByStorageId(resource.section(QLatin1Char(':'), 1));

        KAStatsFavoritesModel *favoritesModel = m_parentModel ? static_cast<KAStatsFavoritesModel *>(m_parentModel->favoritesModel()) : nullptr;

        return (service && (!favoritesModel || !favoritesModel->isFavorite(service->storageId())));
    }

    return true;
}

bool InvalidAppsFilterProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return (left.row() < right.row());
}

bool GroupSortProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const QString &lResource = sourceModel()->data(left, ResultModel::ResourceRole).toString();
    const QString &rResource = sourceModel()->data(right, ResultModel::ResourceRole).toString();

    if (lResource.startsWith(QLatin1String("applications:")) && !rResource.startsWith(QLatin1String("applications:"))) {
        return true;
    } else if (!lResource.startsWith(QLatin1String("applications:")) && rResource.startsWith(QLatin1String("applications:"))) {
        return false;
    }

    return (left.row() < right.row());
}

RecentUsageModel::RecentUsageModel(QObject *parent, IncludeUsage usage, int ordering)
    : ForwardingModel(parent)
    , m_usage(usage)
    , m_ordering((Ordering)ordering)
    , m_complete(false)
    , m_placesModel(new KFilePlacesModel(this))
{
    refresh();
}

RecentUsageModel::~RecentUsageModel()
{
}

void RecentUsageModel::setShownItems(IncludeUsage usage)
{
    if (m_usage == usage) {
        return;
    }

    m_usage = usage;

    Q_EMIT shownItemsChanged();
    refresh();
}

RecentUsageModel::IncludeUsage RecentUsageModel::shownItems() const
{
    return m_usage;
}

QString RecentUsageModel::description() const
{
    switch (m_usage) {
    case AppsAndDocs:
        return i18n("Recently Used");
    case OnlyApps:
        return i18n("Applications");
    case OnlyDocs:
    default:
        return i18n("Files");
    }
}

QString RecentUsageModel::resourceAt(int row) const
{
    return rowValueAt(row, ResultModel::ResourceRole).toString();
}

QVariant RecentUsageModel::rowValueAt(int row, ResultModel::Roles role) const
{
    QSortFilterProxyModel *sourceProxy = qobject_cast<QSortFilterProxyModel *>(sourceModel());

    if (sourceProxy) {
        return sourceProxy->sourceModel()->data(sourceProxy->mapToSource(sourceProxy->index(row, 0)), role).toString();
    }

    return sourceModel()->data(index(row, 0), role);
}

QVariant RecentUsageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const QString &resource = resourceAt(index.row());

    if (resource.startsWith(QLatin1String("applications:"))) {
        return appData(resource, role);
    } else {
        const QString &mimeType = rowValueAt(index.row(), ResultModel::MimeType).toString();
        return docData(resource, role, mimeType);
    }
}

QVariant RecentUsageModel::appData(const QString &resource, int role) const
{
    const QString storageId = resource.section(QLatin1Char(':'), 1);
    KService::Ptr service = KService::serviceByStorageId(storageId);

    QStringList allowedTypes({QLatin1String("Service"), QLatin1String("Application")});

    if (!service || !allowedTypes.contains(service->property(QLatin1String("Type")).toString()) || service->exec().isEmpty()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        AppsModel *parentModel = qobject_cast<AppsModel *>(QObject::parent());

        if (parentModel) {
            return AppEntry::nameFromService(service, (AppEntry::NameFormat)qobject_cast<AppsModel *>(QObject::parent())->appNameFormat());
        } else {
            return AppEntry::nameFromService(service, AppEntry::NameOnly);
        }
    } else if (role == Qt::DecorationRole) {
        return service->icon();
    } else if (role == Kicker::DescriptionRole) {
        return service->comment();
    } else if (role == Kicker::GroupRole) {
        return i18n("Applications");
    } else if (role == Kicker::FavoriteIdRole) {
        return service->storageId();
    } else if (role == Kicker::HasActionListRole) {
        return true;
    } else if (role == Kicker::ActionListRole) {
        QVariantList actionList;

        const QVariantList &jumpList = Kicker::jumpListActions(service);
        if (!jumpList.isEmpty()) {
            actionList << jumpList;
        }

        const QVariantList &recentDocuments = Kicker::recentDocumentActions(service);
        if (!recentDocuments.isEmpty()) {
            actionList << recentDocuments;
        }

        if (!actionList.isEmpty()) {
            actionList << Kicker::createSeparatorActionItem();
        }

        const QVariantMap &forgetAction = Kicker::createActionItem(i18n("Forget Application"), QStringLiteral("edit-clear-history"), QStringLiteral("forget"));
        actionList << forgetAction;

        const QVariantMap &forgetAllAction = Kicker::createActionItem(forgetAllActionName(), QStringLiteral("edit-clear-history"), QStringLiteral("forgetAll"));
        actionList << forgetAllAction;

        return actionList;
    }

    return QVariant();
}

QModelIndex RecentUsageModel::findPlaceForKFileItem(const KFileItem &fileItem) const
{
    const auto index = m_placesModel->closestItem(fileItem.url());
    if (index.isValid()) {
        const auto parentUrl = m_placesModel->url(index);
        if (parentUrl == fileItem.url()) {
            return index;
        }
    }
    return QModelIndex();
}

QVariant RecentUsageModel::docData(const QString &resource, int role, const QString &mimeType) const
{
    QUrl url(resource);

    if (url.scheme().isEmpty()) {
        url.setScheme(QStringLiteral("file"));
    }

    auto getFileItem = [=]() {
        // Avoid calling QT_LSTAT and accessing recent documents
        if (mimeType.simplified().isEmpty()) {
            return KFileItem(url, KFileItem::SkipMimeTypeFromContent);
        } else {
            return KFileItem(url, mimeType);
        }
    };

    if (!url.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        auto fileItem = getFileItem();
        const auto index = findPlaceForKFileItem(fileItem);
        if (index.isValid()) {
            return m_placesModel->text(index);
        }
        return fileItem.text();
    } else if (role == Qt::DecorationRole) {
        auto fileItem = getFileItem();
        const auto index = findPlaceForKFileItem(fileItem);
        if (index.isValid()) {
            return m_placesModel->icon(index);
        }
        return QIcon::fromTheme(fileItem.iconName(), QIcon::fromTheme(QStringLiteral("unknown")));
    } else if (role == Kicker::GroupRole) {
        return i18n("Files");
    } else if (role == Kicker::FavoriteIdRole || role == Kicker::UrlRole) {
        return url.toString();
    } else if (role == Kicker::DescriptionRole) {
        auto fileItem = getFileItem();
        QString desc = fileItem.localPath();

        const auto index = m_placesModel->closestItem(fileItem.url());
        if (index.isValid()) {
            // the current file has a parent in placesModel
            const auto parentUrl = m_placesModel->url(index);
            if (parentUrl == fileItem.url()) {
                // if the current item is a place
                return QString();
            }
            desc.truncate(desc.lastIndexOf(QLatin1Char('/')));
            const auto text = m_placesModel->text(index);
            desc.replace(0, parentUrl.path().length(), text);
        } else {
            // remove filename
            desc.truncate(desc.lastIndexOf(QLatin1Char('/')));
        }
        return desc;
    } else if (role == Kicker::UrlRole) {
        return url;
    } else if (role == Kicker::HasActionListRole) {
        return true;
    } else if (role == Kicker::ActionListRole) {
        auto fileItem = getFileItem();
        QVariantList actionList = Kicker::createActionListForFileItem(fileItem);

        actionList << Kicker::createSeparatorActionItem();

        QVariantMap openParentFolder =
            Kicker::createActionItem(i18n("Open Containing Folder"), QStringLiteral("folder-open"), QStringLiteral("openParentFolder"));
        actionList << openParentFolder;

        QVariantMap forgetAction = Kicker::createActionItem(i18n("Forget File"), QStringLiteral("edit-clear-history"), QStringLiteral("forget"));
        actionList << forgetAction;

        QVariantMap forgetAllAction = Kicker::createActionItem(forgetAllActionName(), QStringLiteral("edit-clear-history"), QStringLiteral("forgetAll"));
        actionList << forgetAllAction;

        return actionList;
    }

    return QVariant();
}

bool RecentUsageModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    Q_UNUSED(argument)

    bool withinBounds = row >= 0 && row < rowCount();

    if (actionId.isEmpty() && withinBounds) {
        const QString &resource = resourceAt(row);
        const QString &mimeType = rowValueAt(row, ResultModel::MimeType).toString();

        if (!resource.startsWith(QLatin1String("applications:"))) {
            const QUrl resourceUrl = docData(resource, Kicker::UrlRole, mimeType).toUrl();

            auto job = new KIO::OpenUrlJob(resourceUrl);
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, QApplication::activeWindow()));
            job->setShowOpenOrExecuteDialog(true);
            job->start();

            return true;
        }

        const QString storageId = resource.section(QLatin1Char(':'), 1);
        KService::Ptr service = KService::serviceByStorageId(storageId);

        if (!service) {
            return false;
        }

        // prevents using a service file that does not support opening a mime type for a file it created
        // for instance a screenshot tool
        if (!mimeType.simplified().isEmpty()) {
            if (!service->hasMimeType(mimeType)) {
                // needs to find the application that supports this mimetype
                service = KApplicationTrader::preferredService(mimeType);

                if (!service) {
                    // no service found to handle the mimetype
                    return false;
                } else {
                    qCWarning(KICKER_DEBUG) << "Preventing the file to open with " << service->desktopEntryName() << "no alternative found";
                }
            }
        }

        auto *job = new KIO::ApplicationLauncherJob(service);
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->start();

        KActivities::ResourceInstance::notifyAccessed(QUrl(QStringLiteral("applications:") + storageId), QStringLiteral("org.kde.plasma.kicker"));

        return true;
    } else if (actionId == QLatin1String("forget") && withinBounds) {
        if (m_activitiesModel) {
            QModelIndex idx = sourceModel()->index(row, 0);
            QSortFilterProxyModel *sourceProxy = qobject_cast<QSortFilterProxyModel *>(sourceModel());

            while (sourceProxy) {
                idx = sourceProxy->mapToSource(idx);
                sourceProxy = qobject_cast<QSortFilterProxyModel *>(sourceProxy->sourceModel());
            }

            static_cast<ResultModel *>(m_activitiesModel.data())->forgetResource(idx.row());
        }

        return false;
    } else if (actionId == QLatin1String("openParentFolder") && withinBounds) {
        const auto url = QUrl::fromUserInput(resourceAt(row));
        KIO::highlightInFileManager({url});
    } else if (actionId == QLatin1String("forgetAll")) {
        if (m_activitiesModel) {
            static_cast<ResultModel *>(m_activitiesModel.data())->forgetAllResources();
        }

        return false;
    } else if (actionId == QLatin1String("_kicker_jumpListAction")) {
        KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(argument.value<KServiceAction>());
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->start();
        return true;
    } else if (withinBounds) {
        const QString &resource = resourceAt(row);

        if (resource.startsWith(QLatin1String("applications:"))) {
            const QString storageId = sourceModel()->data(sourceModel()->index(row, 0), ResultModel::ResourceRole).toString().section(QLatin1Char(':'), 1);
            KService::Ptr service = KService::serviceByStorageId(storageId);

            if (service) {
                return Kicker::handleRecentDocumentAction(service, actionId, argument);
            }
        } else {
            bool close = false;

            QUrl url(sourceModel()->data(sourceModel()->index(row, 0), ResultModel::ResourceRole).toString());

            KFileItem item(url);

            if (Kicker::handleFileItemAction(item, actionId, argument, &close)) {
                return close;
            }
        }
    }

    return false;
}

bool RecentUsageModel::hasActions() const
{
    return rowCount();
}

QVariantList RecentUsageModel::actions() const
{
    QVariantList actionList;

    if (rowCount()) {
        actionList << Kicker::createActionItem(forgetAllActionName(), QStringLiteral("edit-clear-history"), QStringLiteral("forgetAll"));
    }

    return actionList;
}

QString RecentUsageModel::forgetAllActionName() const
{
    switch (m_usage) {
    case AppsAndDocs:
        return i18n("Forget All");
    case OnlyApps:
        return i18n("Forget All Applications");
    case OnlyDocs:
    default:
        return i18n("Forget All Files");
    }
}

void RecentUsageModel::setOrdering(int ordering)
{
    if (ordering == m_ordering)
        return;

    m_ordering = (Ordering)ordering;
    refresh();

    Q_EMIT orderingChanged(ordering);
}

int RecentUsageModel::ordering() const
{
    return m_ordering;
}

void RecentUsageModel::classBegin()
{
}

void RecentUsageModel::componentComplete()
{
    m_complete = true;

    refresh();
}

void RecentUsageModel::refresh()
{
    if (qmlEngine(this) && !m_complete) {
        return;
    }

    QAbstractItemModel *oldModel = sourceModel();
    disconnectSignals();
    setSourceModel(nullptr);
    delete oldModel;

    // clang-format off
    auto query = UsedResources
                    | (m_ordering == Recent ? RecentlyUsedFirst : HighScoredFirst)
                    | Agent::any()
                    | (m_usage == OnlyDocs ? Type::files() : Type::any())
                    | Activity::current();
    // clang-format on

    switch (m_usage) {
    case AppsAndDocs: {
        query = query | Url::startsWith(QStringLiteral("applications:")) | Url::file() | Limit(30);
        break;
    }
    case OnlyApps: {
        query = query | Url::startsWith(QStringLiteral("applications:")) | Limit(15);
        break;
    }
    case OnlyDocs:
    default: {
        query = query | Url::file() | Limit(15);
    }
    }

    m_activitiesModel = new ResultModel(query);
    QAbstractItemModel *model = m_activitiesModel;

    QModelIndex index;

    if (model->canFetchMore(index)) {
        model->fetchMore(index);
    }

    if (m_usage != OnlyDocs) {
        model = new InvalidAppsFilterProxy(this, model);
    }

    if (m_usage == AppsAndDocs) {
        model = new GroupSortProxy(this, model);
    }

    setSourceModel(model);
}

Q_DECLARE_METATYPE(KServiceAction)
