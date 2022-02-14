/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "jumplist.h"

#include <QAction>
#include <QMenu>
#include <QThreadPool>

#include <KActivities/Stats/Cleaning>
#include <KActivities/Stats/ResultSet>
#include <KDesktopFile>
#include <KFileItem>
#include <KFilePlacesModel>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KService/KApplicationTrader>

#include "loadactionsthread_p.h"
#include "log_settings.h"

namespace KAStats = KActivities::Stats;

using namespace KAStats;
using namespace KAStats::Terms;

struct ActionData {
    QIcon icon;
    QString text;
    QUrl url;
    bool isSeparator = false;
    // Actions
    QString storageId;
    QUrl desktopEntryUrl;
    KServiceAction serviceAction;
    // Places
    std::vector<ActionData *> subMenuData;
    bool isShowAllPlace = false;
    // Recent documents
    QString agent;
    QUrl entryPath;
    QString mimeType;
    QString data;
};
Q_DECLARE_METATYPE(ActionData);

namespace JumpList
{

class LoadActionsThread : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit LoadActionsThread(JumpListBackend *parent, const QUrl &launcherUrl);
    void run() override;

Q_SIGNALS:
    void done(const QUrl &launcherUrl, const QVariantList &actions);

private:
    QVariantList systemSettingsActions() const;

    QUrl m_launcherUrl;
    JumpListBackend *p;
};

class LoadPlacesThread : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit LoadPlacesThread(JumpListBackend *parent, const QUrl &launcherUrl, bool showAllPlaces);
    void run() override;

Q_SIGNALS:
    void done(const QUrl &launcherUrl, const QVariantList &actions);

private:
    QUrl m_launcherUrl;
    bool m_showAllPlaces;
    JumpListBackend *p;
};

class LoadRecentDocumentsThread : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit LoadRecentDocumentsThread(JumpListBackend *parent, const QUrl &launcherUrl);
    void run() override;

Q_SIGNALS:
    void done(const QUrl &launcherUrl, const QVariantList &actions);

private:
    QUrl m_launcherUrl;
    JumpListBackend *p;
};

class JumpListBackendPrivate : public QObject
{
    Q_OBJECT

public:
    explicit JumpListBackendPrivate(JumpListBackend *parent);
    ~JumpListBackendPrivate() override;

    void loadActions(const QUrl &launcherUrl);
    void loadPlaces(const QUrl &launcherUrl, bool showAllPlaces);
    void loadRecentDocuments(const QUrl &launcherUrl);

    QHash<QUrl /* launcherUrl */, QVariantList> m_actions;
    QHash<QUrl /* launcherUrl */, QVariantList> m_places;
    QHash<QUrl /* launcherUrl */, QVariantList> m_recentDocuments;

private Q_SLOTS:
    void slotActionsLoaded(const QUrl &launcherUrl, const QVariantList &actions);
    void slotPlacesLoaded(const QUrl &launcherUrl, const QVariantList &actions);
    void slotRecentDocumentsLoaded(const QUrl &launcherUrl, const QVariantList &actions);

private:
    void checkOtherLists(const QUrl &launcherUrl);

    JumpListBackend *q;
};

LoadActionsThread::LoadActionsThread(JumpListBackend *parent, const QUrl &launcherUrl)
    : m_launcherUrl(launcherUrl)
    , p(parent)
{
}

QVariantList LoadActionsThread::systemSettingsActions() const
{
    QVariantList actions;

    auto query = AllResources | Agent(QStringLiteral("org.kde.systemsettings")) | HighScoredFirst | Limit(5);

    ResultSet results(query);

    QStringList ids;
    for (const ResultSet::Result &result : results) {
        ids << QUrl(result.resource()).path();
    }

    if (ids.count() < 5) {
        // We'll load the default set of settings from its jump list actions.
        return actions;
    }

    for (const QString &id : std::as_const(ids)) {
        KService::Ptr service = KService::serviceByStorageId(id);
        if (!service || !service->isValid()) {
            continue;
        }

        ActionData action;
        action.text = service->name();
        action.icon = QIcon::fromTheme(service->icon());
        action.storageId = id;

        actions << QVariant::fromValue<ActionData>(action);
    }

    return actions;
}

void LoadActionsThread::run()
{
    QVariantList actions;

    QUrl desktopEntryUrl = tryDecodeApplicationsUrl(m_launcherUrl);

    if (!desktopEntryUrl.isValid() || !desktopEntryUrl.isLocalFile() || !KDesktopFile::isDesktopFile(desktopEntryUrl.toLocalFile())) {
        Q_EMIT done(m_launcherUrl, actions);
        return;
    }

    const KService::Ptr service = KService::serviceByDesktopPath(desktopEntryUrl.toLocalFile());
    if (!service) {
        Q_EMIT done(m_launcherUrl, actions);
        return;
    }

    if (service->storageId() == QLatin1String("systemsettings.desktop")) {
        actions = systemSettingsActions();
        if (!actions.isEmpty()) {
            Q_EMIT done(m_launcherUrl, actions);
            return;
        }
    }

    const auto jumpListActions = service->actions();

    for (const KServiceAction &serviceAction : jumpListActions) {
        if (serviceAction.noDisplay()) {
            continue;
        }

        ActionData action;
        action.text = serviceAction.text();
        action.icon = QIcon::fromTheme(serviceAction.icon());
        action.serviceAction = serviceAction;

        actions << QVariant::fromValue<ActionData>(action);
    }

    Q_EMIT done(m_launcherUrl, actions);
}

LoadPlacesThread::LoadPlacesThread(JumpListBackend *parent, const QUrl &launcherUrl, bool showAllPlaces)
    : m_launcherUrl(launcherUrl)
    , m_showAllPlaces(showAllPlaces)
    , p(parent)
{
}

void LoadPlacesThread::run()
{
    QVariantList actions;

    QUrl desktopEntryUrl = tryDecodeApplicationsUrl(m_launcherUrl);

    if (!desktopEntryUrl.isValid() || !desktopEntryUrl.isLocalFile() || !KDesktopFile::isDesktopFile(desktopEntryUrl.toLocalFile())) {
        Q_EMIT done(m_launcherUrl, actions);
        return;
    }

    // Since we can't have dynamic jump list actions, at least add the user's "Places" for file managers.
    if (!applicationCategories(m_launcherUrl).contains(QLatin1String("FileManager"))) {
        Q_EMIT done(m_launcherUrl, actions);
        return;
    }

    QString previousGroup;

    std::unique_ptr<KFilePlacesModel> placesModel(new KFilePlacesModel());
    for (int i = 0; i < placesModel->rowCount(); ++i) {
        QModelIndex idx = placesModel->index(i, 0);

        if (placesModel->isHidden(idx)) {
            continue;
        }

        ActionData placeAction;
        placeAction.text = idx.data(Qt::DisplayRole).toString();
        placeAction.icon = idx.data(Qt::DecorationRole).value<QIcon>();
        placeAction.url = idx.data(KFilePlacesModel::UrlRole).toUrl();
        placeAction.desktopEntryUrl = desktopEntryUrl;

        const QString &groupName = idx.data(KFilePlacesModel::GroupRole).toString();
        if (previousGroup.isEmpty()) { // Skip first group heading.
            previousGroup = groupName;
        }

        // Put all subsequent categories into a submenu.
        ActionData subMenuAction;
        if (previousGroup != groupName) {
            subMenuAction.text = groupName;
            actions << QVariant::fromValue<ActionData>(subMenuAction);

            previousGroup = groupName;
        }

        if (!subMenuAction.text.isEmpty()) {
            subMenuAction.subMenuData.emplace_back(new ActionData(placeAction));
        } else {
            actions << QVariant::fromValue<ActionData>(placeAction);
        }
    }

    // There is nothing more frustrating than having a "More" entry that ends up showing just one or two
    // additional entries. Therefore we truncate to max. 5 entries only if there are more than 7 in total.
    if (!m_showAllPlaces && actions.count() > 7) {
        const int totalActionCount = actions.count();

        while (actions.count() > 5) {
            actions.removeLast();
        }

        ActionData action;
        action.text = i18ncp("Show all user Places", "%1 more Place", "%1 more Places", totalActionCount - actions.count());
        action.isShowAllPlace = true;
        actions << QVariant::fromValue<ActionData>(action);
    }

    Q_EMIT done(m_launcherUrl, actions);
}

LoadRecentDocumentsThread::LoadRecentDocumentsThread(JumpListBackend *parent, const QUrl &launcherUrl)
    : m_launcherUrl(launcherUrl)
    , p(parent)
{
}

void LoadRecentDocumentsThread::run()
{
    QVariantList actions;
    QUrl desktopEntryUrl = tryDecodeApplicationsUrl(m_launcherUrl);

    if (!desktopEntryUrl.isValid() || !desktopEntryUrl.isLocalFile() || !KDesktopFile::isDesktopFile(desktopEntryUrl.toLocalFile())) {
        Q_EMIT done(m_launcherUrl, actions);
        return;
    }

    QString desktopName = desktopEntryUrl.fileName();
    QString storageId = desktopName;

    if (storageId.endsWith(QLatin1String(".desktop"))) {
        storageId = storageId.left(storageId.length() - 8);
    }

    auto query = UsedResources | RecentlyUsedFirst | Agent(storageId) | Type::files() | Activity::current() | Url::file();

    ResultSet results(query);

    ResultSet::const_iterator resultIt = results.begin();

    int actionCount = 0;

    while (actionCount < 5 && resultIt != results.end()) {
        const QString resource = (*resultIt).resource();
        const QString mimetype = (*resultIt).mimetype();
        ++resultIt;

        const QUrl url = QUrl::fromLocalFile(resource);

        if (!url.isValid()) {
            continue;
        }

        const KFileItem fileItem(url, KFileItem::SkipMimeTypeFromContent);

        ActionData action;
        action.text = url.fileName();
        action.icon = QIcon::fromTheme(fileItem.iconName(), QIcon::fromTheme(QStringLiteral("unknown")));
        action.agent = storageId;
        action.url = desktopEntryUrl;
        action.mimeType = mimetype;
        action.data = resource;

        actions << QVariant::fromValue<ActionData>(action);

        ++actionCount;
    }

    Q_EMIT done(m_launcherUrl, actions);
}

JumpListBackendPrivate::JumpListBackendPrivate(JumpListBackend *parent)
    : QObject(parent)
    , q(parent)
{
}

JumpListBackendPrivate::~JumpListBackendPrivate()
{
}

void JumpListBackendPrivate::loadActions(const QUrl &launcherUrl)
{
    m_actions.remove(launcherUrl);

    LoadActionsThread *actionsThread = new LoadActionsThread(q, launcherUrl);
    connect(actionsThread, &LoadActionsThread::done, this, &JumpListBackendPrivate::slotActionsLoaded);
    QThreadPool::globalInstance()->start(actionsThread);
}

void JumpListBackendPrivate::loadPlaces(const QUrl &launcherUrl, bool showAllPlaces)
{
    const auto it = m_places.constFind(launcherUrl);
    if (it != m_places.cend()) {
        for (const auto &action : std::as_const(*it)) {
            const auto data = action.value<ActionData>();
            std::for_each(data.subMenuData.cbegin(), data.subMenuData.cend(), [](ActionData *data) {
                delete data;
            });
        }
        m_places.erase(it);
    }

    LoadPlacesThread *placesThread = new LoadPlacesThread(q, launcherUrl, showAllPlaces);
    connect(placesThread, &LoadPlacesThread::done, this, &JumpListBackendPrivate::slotPlacesLoaded);
    QThreadPool::globalInstance()->start(placesThread);
}

void JumpListBackendPrivate::loadRecentDocuments(const QUrl &launcherUrl)
{
    m_recentDocuments.remove(launcherUrl);

    // Actions depend on ApplicationActionsModel::handleRecentDocumentAction()
    LoadRecentDocumentsThread *recentDocumentsThread = new LoadRecentDocumentsThread(q, launcherUrl);
    connect(recentDocumentsThread, &LoadRecentDocumentsThread::done, this, &JumpListBackendPrivate::slotRecentDocumentsLoaded);
    QThreadPool::globalInstance()->start(recentDocumentsThread);
}

void JumpListBackendPrivate::slotActionsLoaded(const QUrl &launcherUrl, const QVariantList &actions)
{
    m_actions[launcherUrl] = actions;
    Q_EMIT q->actionsChanged(launcherUrl);

    checkOtherLists(launcherUrl);
}

void JumpListBackendPrivate::slotPlacesLoaded(const QUrl &launcherUrl, const QVariantList &actions)
{
    m_places[launcherUrl] = actions;
    Q_EMIT q->placesChanged(launcherUrl);

    checkOtherLists(launcherUrl);
}

void JumpListBackendPrivate::slotRecentDocumentsLoaded(const QUrl &launcherUrl, const QVariantList &actions)
{
    m_recentDocuments[launcherUrl] = actions;
    Q_EMIT q->recentDocumentsChanged(launcherUrl);

    checkOtherLists(launcherUrl);
}

void JumpListBackendPrivate::checkOtherLists(const QUrl &launcherUrl)
{
    if (!m_recentDocuments.contains(launcherUrl) || !m_places.contains(launcherUrl) || !m_actions.contains(launcherUrl)) {
        return;
    }

    Q_EMIT q->listReady(launcherUrl);
}

JumpListBackend::JumpListBackend(QObject *parent)
    : QObject(parent)
    , d(new JumpListBackendPrivate(this))
{
}

JumpListBackend::~JumpListBackend()
{
}

void JumpListBackend::loadJumpList(const QUrl &launcherUrl, bool showAllPlaces) const
{
    d->loadRecentDocuments(launcherUrl);
    d->loadPlaces(launcherUrl, showAllPlaces);
    d->loadActions(launcherUrl);
}

QVariantList JumpListBackend::actions(const QUrl &launcherUrl, QObject *parent) const
{
    const auto &data = d->m_actions.value(launcherUrl);
    QVariantList actions;

    for (const auto &d : data) {
        const auto &ad = d.value<ActionData>();
        if (!ad.storageId.isEmpty()) {
            // From systemSettings
            QAction *action = new QAction(ad.icon, ad.text, parent);

            connect(action, &QAction::triggered, this, [&ad]() {
                auto *job = new KIO::ApplicationLauncherJob(KService::serviceByStorageId(ad.storageId));
                auto *delegate = new KNotificationJobUiDelegate;
                delegate->setAutoErrorHandlingEnabled(true);
                job->setUiDelegate(delegate);
                job->start();
            });

            actions << QVariant::fromValue<QAction *>(action);
        } else {
            QAction *action = new QAction(ad.icon, ad.text, parent);

            if (ad.serviceAction.isSeparator()) {
                action->setSeparator(true);
            }

            connect(action, &QAction::triggered, this, [&ad]() {
                auto *job = new KIO::ApplicationLauncherJob(ad.serviceAction);
                auto *delegate = new KNotificationJobUiDelegate;
                delegate->setAutoErrorHandlingEnabled(true);
                job->setUiDelegate(delegate);
                job->start();
            });

            actions << QVariant::fromValue<QAction *>(action);
        }
    }

    return actions;
}

QVariantList JumpListBackend::places(const QUrl &launcherUrl, QObject *parent) const
{
    const auto &data = d->m_places.value(launcherUrl);
    QVariantList actions;

    auto createAction = [this, parent](const ActionData &ad) {
        QAction *placeAction = new QAction(ad.icon, ad.text, parent);

        if (ad.isShowAllPlace) {
            // connect(placeAction, &QAction::triggered, this, &JumpListBackend::showAllPlaces);
        } else {
            connect(placeAction, &QAction::triggered, this, [&ad] {
                KService::Ptr service = KService::serviceByDesktopPath(ad.desktopEntryUrl.toLocalFile());
                if (!service) {
                    return;
                }

                auto *job = new KIO::ApplicationLauncherJob(service);
                auto *delegate = new KNotificationJobUiDelegate;
                delegate->setAutoErrorHandlingEnabled(true);
                job->setUiDelegate(delegate);

                job->setUrls({ad.url});
                job->start();
            });
        }

        return placeAction;
    };

    for (const auto &d : data) {
        const auto &ad = d.value<ActionData>();
        if (ad.subMenuData.size() == 0) {
            actions << QVariant::fromValue(createAction(ad));
        } else {
            QAction *subMenuAction = new QAction(ad.text, parent);
            QMenu *subMenu = new QMenu();
            // Cannot parent a QMenu to a QAction, need to delete it manually.
            connect(parent, &QObject::destroyed, subMenu, &QObject::deleteLater);
            subMenuAction->setMenu(subMenu);
            actions << QVariant::fromValue(subMenuAction);

            for (const auto p : std::as_const(ad.subMenuData)) {
                subMenu->addAction(createAction(*p));
            }
        }
    }

    return actions;
}

QVariantList JumpListBackend::recentDocuments(const QUrl &launcherUrl, QObject *parent) const
{
    const auto &data = d->m_recentDocuments.value(launcherUrl);
    QVariantList actions;

    for (const auto &d : data) {
        const auto &ad = d.value<ActionData>();
        QAction *action = new QAction(parent);
        action->setText(ad.text);
        action->setIcon(ad.icon);
        action->setProperty("agent", ad.agent);
        action->setProperty("entryPath", ad.url);
        action->setProperty("mimeType", ad.mimeType);
        action->setData(ad.data);
        connect(action, &QAction::triggered, this, &JumpListBackend::handleRecentDocumentAction);

        actions << QVariant::fromValue<QAction *>(action);
    }

    if (data.size() > 0) {
        QAction *separatorAction = new QAction(parent);
        separatorAction->setSeparator(true);
        actions << QVariant::fromValue<QAction *>(separatorAction);

        QAction *action = new QAction(parent);
        action->setText(i18n("Forget Recent Files"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
        action->setProperty("agent", data.constFirst().value<ActionData>().agent);
        connect(action, &QAction::triggered, this, &JumpListBackend::handleRecentDocumentAction);
        actions << QVariant::fromValue<QAction *>(action);
    }

    return actions;
}

void JumpListBackend::handleRecentDocumentAction() const
{
    const QAction *action = qobject_cast<QAction *>(sender());

    if (!action) {
        return;
    }

    const QString agent = action->property("agent").toString();

    if (agent.isEmpty()) {
        return;
    }

    const QString desktopPath = action->property("entryPath").toUrl().toLocalFile();
    const QString resource = action->data().toString();

    if (desktopPath.isEmpty() || resource.isEmpty()) {
        auto query = UsedResources | Agent(agent) | Type::any() | Activity::current() | Url::file();

        KAStats::forgetResources(query);

        return;
    }

    KService::Ptr service = KService::serviceByDesktopPath(desktopPath);

    if (!service) {
        return;
    }

    // prevents using a service file that does not support opening a mime type for a file it created
    // for instance spectacle
    const auto mimetype = action->property("mimeType").toString();
    if (!mimetype.isEmpty()) {
        if (!service->hasMimeType(mimetype)) {
            // needs to find the application that supports this mimetype
            service = KApplicationTrader::preferredService(mimetype);

            if (!service) {
                // no service found to handle the mimetype
                return;
            } else {
                qCWarning(JUMPLIST_DEBUG) << "Preventing the file to open with " << service->desktopEntryName() << "no alternative found";
            }
        }
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    auto *delegate = new KNotificationJobUiDelegate;
    delegate->setAutoErrorHandlingEnabled(true);
    job->setUiDelegate(delegate);

    job->setUrls({QUrl(resource)});
    job->start();
}

void JumpListBackend::showAllPlaces(const QUrl &launcherUrl) const
{
    d->loadPlaces(launcherUrl, true);
}
}
#include "jumplist.moc"