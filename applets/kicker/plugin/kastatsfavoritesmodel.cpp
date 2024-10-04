/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2016-2017 Ivan Cukic <ivan.cukic@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kastatsfavoritesmodel.h"
#include "actionlist.h"
#include "appentry.h"
#include "debug.h"
#include "fileentry.h"

#include <QFileInfo>
#include <QSortFilterProxyModel>
#include <QTimer>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KProtocolInfo>
#include <KSharedConfig>
#include <KStringHandler>
#include <KSycoca>

#include <PlasmaActivities/Consumer>
#include <PlasmaActivities/Stats/Query>
#include <PlasmaActivities/Stats/ResultSet>
#include <PlasmaActivities/Stats/ResultWatcher>
#include <PlasmaActivities/Stats/Terms>
#include <qnamespace.h>

#include "config-KDECI_BUILD.h"

namespace KAStats = KActivities::Stats;

using namespace KAStats;
using namespace KAStats::Terms;
using namespace Qt::StringLiterals;

#define AGENT_APPLICATIONS QStringLiteral("org.kde.plasma.favorites.applications")
#define AGENT_DOCUMENTS QStringLiteral("org.kde.plasma.favorites.documents")

QString agentForUrl(const QString &url)
{
    QUrl u(url);
    // clang-format off
    return url.startsWith(QLatin1String("preferred:"))
                ? AGENT_APPLICATIONS
         : url.startsWith(QLatin1String("applications:"))
                ? AGENT_APPLICATIONS
         : (url.startsWith(QLatin1Char('/')) && !url.endsWith(QLatin1String(".desktop")))
                ? AGENT_DOCUMENTS
         : (url.startsWith(QLatin1String("file:/")) && !url.endsWith(QLatin1String(".desktop")))
                ? AGENT_DOCUMENTS
         : (u.scheme() != QLatin1String("file") && !u.scheme().isEmpty() && KProtocolInfo::isKnownProtocol(u.scheme()))
                  ? AGENT_DOCUMENTS
         // use applications as the default
                : AGENT_APPLICATIONS;
    // clang-format on
}

class KAStatsFavoritesModel::Private : public QAbstractListModel
{
public:
    class NormalizedId
    {
    public:
        NormalizedId()
        {
        }

        NormalizedId(const Private *parent, const QString &id)
        {
            if (id.isEmpty())
                return;

            std::shared_ptr<AbstractEntry> entry = nullptr;

            if (auto it = parent->m_itemEntries.find(id); it != parent->m_itemEntries.cend()) {
                entry = it->second;
            } else {
                // This entry is not cached - it is temporary,
                // so let's clean up when we exit this function
                entry = parent->entryForResource(id);
            }

            if (!entry || !entry->isValid()) {
                qCWarning(KICKER_DEBUG) << "Entry is not valid" << id << entry.get();
                m_id = id;
                return;
            }

            const auto url = entry->url();

            qCDebug(KICKER_DEBUG) << "Original id is: " << id << ", and the url is" << url;

            // Preferred applications need special handling
            if (entry->id().startsWith(QLatin1String("preferred:"))) {
                m_id = entry->id();
                return;
            }

            // If this is an application, use the applications:-format url
            auto appEntry = dynamic_cast<AppEntry *>(entry.get());
            if (appEntry && !appEntry->menuId().isEmpty()) {
                m_id = QLatin1String("applications:") + appEntry->menuId();
                return;
            }

            // We want to resolve symbolic links not to have two paths
            // refer to the same .desktop file
            if (url.isLocalFile()) {
                QFileInfo file(url.toLocalFile());

                if (file.exists()) {
                    m_id = QUrl::fromLocalFile(file.canonicalFilePath()).toString();
                    return;
                }
            }

            // If this is a file, we should have already covered it
            if (url.scheme() == QLatin1String("file")) {
                return;
            }

            m_id = url.toString();
        }

        const QString &value() const
        {
            return m_id;
        }

        bool operator==(const NormalizedId &other) const
        {
            return m_id == other.m_id;
        }

    private:
        QString m_id;
    };

    NormalizedId normalizedId(const QString &id) const
    {
        return NormalizedId(this, id);
    }

    std::shared_ptr<AbstractEntry> entryForResource(const QString &resource, const QString &mimeType = QString()) const
    {
        using SP = std::shared_ptr<AbstractEntry>;

        const auto agent = agentForUrl(resource);

        if (agent == AGENT_DOCUMENTS) {
            if (resource.startsWith(QLatin1String("/"))) {
                return SP(new FileEntry(q, QUrl::fromLocalFile(resource), mimeType));
            } else {
                return SP(new FileEntry(q, QUrl(resource), mimeType));
            }

        } else if (agent == AGENT_APPLICATIONS) {
            if (resource.startsWith(QLatin1String("applications:"))) {
                return SP(new AppEntry(q, resource.mid(13)));
            } else {
                return SP(new AppEntry(q, resource));
            }

        } else {
            return {};
        }
    }

    Private(KAStatsFavoritesModel *parent, const QString &clientId)
        : q(parent)
        , m_query(LinkedResources | Agent{AGENT_APPLICATIONS, AGENT_DOCUMENTS} | Type::any() | Activity::current() | Activity::global() | Limit(100))
        , m_watcher(m_query)
        , m_clientId(clientId)
    {
        // Connecting the watcher
        connect(&m_watcher, &ResultWatcher::resultLinked, parent, [this](const QString &resource) {
            addResult(resource, -1);
        });

        connect(&m_watcher, &ResultWatcher::resultUnlinked, parent, [this](const QString &resource) {
            removeResult(resource);
        });
        connect(
            KSycoca::self(),
            &KSycoca::databaseChanged,
            this,
            [this]() {
                QStringList keys;
                // ResultWatcher can emit resultUnlinked when AppEntry::reload() is reparsing configuration which will modify m_itemEntries
                // https://crash-reports.kde.org/organizations/kde/issues/23450/
                const auto itemEntries = m_itemEntries;
                for (auto it = itemEntries.cbegin(); it != itemEntries.cend(); it = std::next(it)) {
                    it->second->reload();
                    if (!it->second->isValid()) {
                        keys << it->first;
                    }
                }
                if (!keys.isEmpty()) {
                    for (const QString &key : keys) {
                        removeResult(key);
                    }
                }
            },
            Qt::QueuedConnection);

        // Loading the items order
        const auto cfg = KSharedConfig::openConfig(QStringLiteral("kactivitymanagerd-statsrc"));

        // We want first to check whether we have an ordering for this activity.
        // If not, we will try to get a global one for this applet

        const QString thisGroupName = u"Favorites-" + clientId + u'-' + m_activities.currentActivity();
        const QString globalGroupName = u"Favorites-" + clientId + u"-global";

        KConfigGroup thisCfgGroup(cfg, thisGroupName);
        KConfigGroup globalCfgGroup(cfg, globalGroupName);

        QStringList ordering = thisCfgGroup.readEntry("ordering", QStringList()) + globalCfgGroup.readEntry("ordering", QStringList());
        // Normalizing all the ids
        std::transform(ordering.begin(), ordering.end(), ordering.begin(), [&](const QString &item) {
            return normalizedId(item).value();
        });

        qCDebug(KICKER_DEBUG) << "Loading the ordering " << ordering;

        // Loading the results without emitting any model signals
        qCDebug(KICKER_DEBUG) << "Query is" << m_query;
        ResultSet results(m_query);

        for (const auto &result : results) {
            qCDebug(KICKER_DEBUG) << "Got " << result.resource() << " -->";
            addResult(result.resource(), -1, false, result.mimetype());
        }

        if (ordering.length() == 0) {
            // try again with other applets with highest instance number which has the matching apps
            qCDebug(KICKER_DEBUG) << "No ordering for this applet found, trying others";
            const auto allGroups = cfg->groupList();
            int instanceHighest = -1;
            for (const auto &groupName : allGroups) {
                if (groupName.contains(QStringLiteral(".favorites.instance-"))
                    && (groupName.endsWith(QStringLiteral("-global")) || groupName.endsWith(m_activities.currentActivity()))) {
                    // the group names look like "Favorites-org.kde.plasma.kicker.favorites.instance-58-1bd5bb42-187c-4c77-a746-c9644c5da866"
                    const QStringList split = groupName.split(QStringLiteral("-"));
                    if (split.length() >= 3) {
                        bool ok;
                        int instanceN = split[2].toInt(&ok);
                        if (!ok) {
                            continue;
                        }
                        auto groupOrdering = KConfigGroup(cfg, groupName).readEntry("ordering", QStringList());
                        if (groupOrdering.length() != m_items.length()) {
                            continue;
                        }
                        std::transform(groupOrdering.begin(), groupOrdering.end(), groupOrdering.begin(), [&](const QString &item) {
                            return normalizedId(item).value();
                        });
                        for (const auto &item : m_items) {
                            if (!groupOrdering.contains(item.value())) {
                                continue;
                            }
                        }
                        if (instanceHighest == instanceN) {
                            // we got a -global as well as -{activity uuid}
                            // we add them
                            if (groupName.endsWith(QStringLiteral("-global"))) {
                                ordering += groupOrdering;
                            } else {
                                ordering = groupOrdering + ordering;
                            }
                            qCDebug(KICKER_DEBUG) << "adding ordering from: " << groupName;
                        } else if (instanceN > instanceHighest) {
                            instanceHighest = instanceN;
                            ordering = (groupOrdering.length() != 0) ? groupOrdering : ordering;
                            qCDebug(KICKER_DEBUG) << "taking ordering from: " << groupName;
                        }
                    }
                }
            }
        }

        // Sorting the items in the cache
        std::sort(m_items.begin(), m_items.end(), [&](const NormalizedId &left, const NormalizedId &right) {
            auto leftIndex = ordering.indexOf(left.value());
            auto rightIndex = ordering.indexOf(right.value());
            // clang-format off
                    return (leftIndex == -1 && rightIndex == -1) ?
                               left.value() < right.value() :

                           (leftIndex == -1) ?
                               false :

                           (rightIndex == -1) ?
                               true :

                           // otherwise
                               leftIndex < rightIndex;
            // clang-format on
        });

        // Debugging:
        QList<QString> itemStrings(m_items.size());
        std::transform(m_items.cbegin(), m_items.cend(), itemStrings.begin(), [](const NormalizedId &item) {
            return item.value();
        });
        qCDebug(KICKER_DEBUG) << "After ordering: " << itemStrings;
    }

    void addResult(const QString &_resource, int index, bool notifyModel = true, const QString &mimeType = QString())
    {
        // We want even files to have a proper URL
        const auto resource = _resource.startsWith(QLatin1Char('/')) ? QUrl::fromLocalFile(_resource).toString() : _resource;

        qCDebug(KICKER_DEBUG) << "Adding result" << resource << "already present?" << m_itemEntries.contains(resource);

        if (m_itemEntries.contains(resource))
            return;

        auto entry = entryForResource(resource, mimeType);

        if (!entry || !entry->isValid()) {
            qCDebug(KICKER_DEBUG) << "Entry is not valid!" << resource;
            return;
        }

        if (index == -1) {
            index = m_items.count();
        }

        if (notifyModel) {
            beginInsertRows(QModelIndex(), index, index);
        }

        auto url = entry->url();

        m_itemEntries[resource] = m_itemEntries[entry->id()] = m_itemEntries[url.toString()] = entry;
        if (!url.toLocalFile().isEmpty()) {
            m_itemEntries[url.toLocalFile()] = entry;
        }

        auto normalized = normalizedId(resource);
        m_items.insert(index, normalized);
        m_itemEntries[normalized.value()] = entry;

        if (notifyModel) {
            endInsertRows();
            saveOrdering();
        }
    }

    void removeResult(const QString &resource)
    {
        const auto normalized = normalizedId(resource);

        // If we know this item will not really be removed,
        // but only that activities it is on have changed,
        // lets leave it
        if (m_ignoredItems.contains(normalized.value())) {
            m_ignoredItems.removeAll(normalized.value());
            return;
        }

        qCDebug(KICKER_DEBUG) << "Removing result" << resource;

        auto index = m_items.indexOf(normalized);

        if (index == -1)
            return;

        beginRemoveRows(QModelIndex(), index, index);
        m_items.removeAt(index);

        // Removing the entry from the cache
        for (auto it = m_itemEntries.cbegin(); it != m_itemEntries.cend();) {
            if (it->second->id() == resource) {
                it = m_itemEntries.erase(it);
            } else {
                it = std::next(it);
            }
        }

        endRemoveRows();

        saveOrdering();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;

        return m_items.count();
    }

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const override
    {
        if (item.parent().isValid())
            return QVariant();

        const auto index = item.row();

        // If index is out of bounds, m_items.value returns default constructed value.
        // In that case, m_itemEntries.value will return default constructed value which is nullptr.
        auto it = m_itemEntries.find(m_items.value(index).value());
        if (it == m_itemEntries.cend()) {
            return QVariant();
        }
        const auto &entry = it->second;
        // clang-format off
        return role == Qt::DisplayRole ? entry->name()
             : role == Kicker::CompactNameRole ? entry->compactName()
             : role == Kicker::CompactNameWrappedRole ? KStringHandler::preProcessWrap(entry->compactName())
             : role == Kicker::DisplayWrappedRole ? KStringHandler::preProcessWrap(entry->name())
             : role == Qt::DecorationRole ? entry->icon()
             : role == Kicker::DescriptionRole ? entry->description()
             : role == Kicker::FavoriteIdRole ? entry->id()
             : role == Kicker::UrlRole ? entry->url()
             : role == Kicker::HasActionListRole ? entry->hasActions()
             : role == Kicker::ActionListRole ? entry->actions()
             : QVariant();
        // clang-format on
    }

    bool trigger(int row, const QString &actionId, const QVariant &argument)
    {
        if (row < 0 || row >= rowCount()) {
            return false;
        }

        const QString id = data(index(row, 0), Kicker::UrlRole).toString();
        if (m_itemEntries.contains(id)) {
            return m_itemEntries.at(id)->run(actionId, argument);
        }
        // Entries with preferred:// can be changed by the user, BUG: 416161
        // then the list of entries could be out of sync
        auto it = m_itemEntries.find(m_items.value(row).value());
        if (it == m_itemEntries.cend()) {
            return false;
        }
        const auto &entry = it->second;
        if (QUrl(entry->id()).scheme() == QLatin1String("preferred")) {
            return entry->run(actionId, argument);
        }
        return false;
    }

    void move(int from, int to)
    {
        if (from < 0)
            return;
        if (from >= m_items.count())
            return;
        if (to < 0)
            return;
        if (to >= m_items.count())
            return;

        if (from == to)
            return;

        const int modelTo = to + (to > from ? 1 : 0);

        if (q->beginMoveRows(QModelIndex(), from, from, QModelIndex(), modelTo)) {
            m_items.move(from, to);
            q->endMoveRows();

            qCDebug(KICKER_DEBUG) << "Save ordering (from Private::move) -->";
            saveOrdering();
        }
    }

    void saveOrdering()
    {
        QStringList ids;

        for (const auto &item : std::as_const(m_items)) {
            ids << item.value();
        }

        qCDebug(KICKER_DEBUG) << "Save ordering (from Private::saveOrdering) -->";
        saveOrdering(ids, m_clientId, m_activities.currentActivity());
    }

    static void saveOrdering(const QStringList &ids, const QString &clientId, const QString &currentActivity)
    {
        const auto cfg = KSharedConfig::openConfig(QStringLiteral("kactivitymanagerd-statsrc"));

        QStringList activities{currentActivity, QStringLiteral("global")};

        qCDebug(KICKER_DEBUG) << "Saving ordering for" << currentActivity << "and global" << ids;

        for (const auto &activity : activities) {
            const QString groupName = u"Favorites-" + clientId + u'-' + activity;

            KConfigGroup cfgGroup(cfg, groupName);

            cfgGroup.writeEntry("ordering", ids);
        }

        cfg->sync();
    }

    KAStatsFavoritesModel *const q;
    KActivities::Consumer m_activities;
    Query m_query;
    ResultWatcher m_watcher;
    QString m_clientId;

    QList<NormalizedId> m_items;
    std::unordered_map<QString, std::shared_ptr<AbstractEntry>> m_itemEntries; // Don't use QHash: https://bugreports.qt.io/browse/QTBUG-129293
    QStringList m_ignoredItems;
};

KAStatsFavoritesModel::KAStatsFavoritesModel(QObject *parent)
    : PlaceholderModel(parent)
    , d(nullptr) // we have no client id yet
    , m_enabled(true)
    , m_maxFavorites(-1)
    , m_activities(new KActivities::Consumer(this))
{
    connect(m_activities, &KActivities::Consumer::currentActivityChanged, this, [&](const QString &currentActivity) {
        qCDebug(KICKER_DEBUG) << "Activity just got changed to" << currentActivity;
        Q_UNUSED(currentActivity);
        if (d && m_activities->serviceStatus() == KActivities::Consumer::Running /*PLASMA-WORKSPACE-125Z*/) {
            auto clientId = d->m_clientId;
            initForClient(clientId);
        }
    });
}

KAStatsFavoritesModel::~KAStatsFavoritesModel()
{
    delete d;
}

void KAStatsFavoritesModel::initForClient(const QString &clientId)
{
    qCDebug(KICKER_DEBUG) << "initForClient" << clientId;

    setSourceModel(nullptr);
    delete d;
    d = new Private(this, clientId);

    setSourceModel(d);
}

QString KAStatsFavoritesModel::description() const
{
    return i18n("Favorites");
}

bool KAStatsFavoritesModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    return d && d->trigger(row, actionId, argument);
}

bool KAStatsFavoritesModel::enabled() const
{
    return m_enabled;
}

int KAStatsFavoritesModel::maxFavorites() const
{
    return m_maxFavorites;
}

void KAStatsFavoritesModel::setMaxFavorites(int max)
{
    Q_UNUSED(max);
}

void KAStatsFavoritesModel::setEnabled(bool enable)
{
    if (m_enabled != enable) {
        m_enabled = enable;

        Q_EMIT enabledChanged();
    }
}

QStringList KAStatsFavoritesModel::favorites() const
{
    qCWarning(KICKER_DEBUG) << "KAStatsFavoritesModel::favorites returns nothing, it is here just to keep the API backwards-compatible";
    return QStringList();
}

void KAStatsFavoritesModel::setFavorites(const QStringList &favorites)
{
    Q_UNUSED(favorites);
    qCWarning(KICKER_DEBUG) << "KAStatsFavoritesModel::setFavorites is ignored";
}

bool KAStatsFavoritesModel::isFavorite(const QString &id) const
{
    return d && d->m_itemEntries.contains(id);
}

void KAStatsFavoritesModel::portOldFavorites(const QStringList &_ids)
{
    if (!d)
        return;

    KConfig config(u"kicker-extra-favoritesrc"_s);
    auto group = config.group(u"General"_s);
    auto prepend = group.readXdgListEntry("Prepend", QStringList());
    auto append = group.readXdgListEntry("Append", QStringList());
    auto ignoreDefaults = group.readEntry("IgnoreDefaults", false);

#if BUILD_TESTING
    if (qEnvironmentVariableIsSet("KDECI_BUILD")) {
        prepend = QStringList{
            QLatin1String("org.kde.plasma.emojier.desktop"),
            QLatin1String("linguist5.desktop"),
            QLatin1String("org.qt.linguist6.desktop"),
        };
        ignoreDefaults = true;
        append.clear();
    }
#endif

    const auto ids = prepend + (ignoreDefaults ? QStringList() : _ids) + append;

    qCDebug(KICKER_DEBUG) << "portOldFavorites" << ids;

    const QString activityId = QStringLiteral(":global");
    std::for_each(ids.begin(), ids.end(), [&](const QString &id) {
        addFavoriteTo(id, activityId);
    });

    // Resetting the model
    auto clientId = d->m_clientId;
    setSourceModel(nullptr);
    delete d;
    d = nullptr;

    qCDebug(KICKER_DEBUG) << "Save ordering (from portOldFavorites) -->";
    Private::saveOrdering(ids, clientId, m_activities->currentActivity());

    QTimer::singleShot(500, this, std::bind(&KAStatsFavoritesModel::initForClient, this, clientId));
}

void KAStatsFavoritesModel::addFavorite(const QString &id, int index)
{
    qCDebug(KICKER_DEBUG) << "addFavorite" << id << index << " -->";
    addFavoriteTo(id, QStringLiteral(":global"), index);
}

void KAStatsFavoritesModel::removeFavorite(const QString &id)
{
    qCDebug(KICKER_DEBUG) << "removeFavorite" << id << " -->";
    removeFavoriteFrom(id, QStringLiteral(":any"));
}

void KAStatsFavoritesModel::addFavoriteTo(const QString &id, const QString &activityId, int index)
{
    qCDebug(KICKER_DEBUG) << "addFavoriteTo" << id << activityId << index << " -->";
    addFavoriteTo(id, Activity(activityId), index);
}

void KAStatsFavoritesModel::removeFavoriteFrom(const QString &id, const QString &activityId)
{
    qCDebug(KICKER_DEBUG) << "removeFavoriteFrom" << id << activityId << " -->";
    removeFavoriteFrom(id, Activity(activityId));
}

void KAStatsFavoritesModel::addFavoriteTo(const QString &id, const Activity &activity, int index)
{
    if (!d || id.isEmpty())
        return;

    Q_ASSERT(!activity.values.isEmpty());

    setDropPlaceholderIndex(-1);

    QStringList matchers{d->m_activities.currentActivity(), QStringLiteral(":global"), QStringLiteral(":current")};
    if (std::find_first_of(activity.values.cbegin(), activity.values.cend(), matchers.cbegin(), matchers.cend()) != activity.values.cend()) {
        d->addResult(id, index);
    }

    const auto url = d->normalizedId(id).value();

    qCDebug(KICKER_DEBUG) << "addFavoriteTo" << id << activity << index << url << " (actual)";

    if (url.isEmpty())
        return;

    d->m_watcher.linkToActivity(QUrl(url), activity, Agent(agentForUrl(url)));
}

void KAStatsFavoritesModel::removeFavoriteFrom(const QString &id, const Activity &activity)
{
    if (!d || id.isEmpty())
        return;

    Q_ASSERT(!activity.values.isEmpty());

    qCDebug(KICKER_DEBUG) << "removeFavoriteFrom" << id << activity;

    if (!isFavorite(id)) {
        return;
    }

    QUrl url = QUrl(id);

    d->m_watcher.unlinkFromActivity(url, activity, Agent(agentForUrl(id)));
}

void KAStatsFavoritesModel::setFavoriteOn(const QString &id, const QString &activityId)
{
    if (!d || id.isEmpty())
        return;

    const auto url = d->normalizedId(id).value();

    qCDebug(KICKER_DEBUG) << "setFavoriteOn" << id << activityId << url << " (actual)";

    qCDebug(KICKER_DEBUG) << "%%%%%%%%%%% Activity is" << activityId;
    if (activityId.isEmpty() || activityId == QLatin1String(":any") || activityId == QLatin1String(":global")
        || activityId == m_activities->currentActivity()) {
        d->m_ignoredItems << url;
    }

    d->m_watcher.unlinkFromActivity(QUrl(url), Activity::any(), Agent(agentForUrl(url)));
    d->m_watcher.linkToActivity(QUrl(url), activityId, Agent(agentForUrl(url)));
}

void KAStatsFavoritesModel::moveRow(int from, int to)
{
    if (!d)
        return;

    d->move(from, to);
}

AbstractModel *KAStatsFavoritesModel::favoritesModel()
{
    return this;
}

void KAStatsFavoritesModel::refresh()
{
    for (auto it = d->m_itemEntries.cbegin(); it != d->m_itemEntries.cend(); it = std::next(it)) {
        it->second->refreshLabels();
    }

    Q_EMIT dataChanged(index(0, 0), index(rowCount(), 0));
}

QObject *KAStatsFavoritesModel::activities() const
{
    return m_activities;
}

QString KAStatsFavoritesModel::activityNameForId(const QString &activityId) const
{
    // It is safe to use a short-lived object here,
    // we are always synced with KAMD in plasma
    KActivities::Info info(activityId);
    return info.name();
}

QStringList KAStatsFavoritesModel::linkedActivitiesFor(const QString &id) const
{
    if (!d) {
        qCDebug(KICKER_DEBUG) << "Linked for" << id << "is empty, no Private instance";
        return {};
    }

    auto url = d->normalizedId(id).value();

    if (url.startsWith(QLatin1String("file:"))) {
        url = QUrl(url).toLocalFile();
    }

    if (url.isEmpty()) {
        qCDebug(KICKER_DEBUG) << "The url for" << id << "is empty";
        return {};
    }

    auto query = LinkedResources | Agent{AGENT_APPLICATIONS, AGENT_DOCUMENTS} | Type::any() | Activity::any() | Url(url) | Limit(100);

    ResultSet results(query);

    for (const auto &result : results) {
        qCDebug(KICKER_DEBUG) << "Returning" << result.linkedActivities() << "for" << id << url;
        return result.linkedActivities();
    }

    qCDebug(KICKER_DEBUG) << "Returning empty list of activities for" << id << url;
    return {};
}
