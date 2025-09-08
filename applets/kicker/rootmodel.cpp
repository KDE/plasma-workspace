/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "rootmodel.h"
#include "actionlist.h"
#include "debug.h"
#include "kastatsfavoritesmodel.h"
#include "recentusagemodel.h"
#include "systemmodel.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QCollator>
#include <QDBusConnection>
#include <QTimer>

#include <chrono>

using namespace std::literals::chrono_literals;

namespace
{
static constexpr int s_rememberUninstalledDays = 3; // How many days we remember any uninstalled app
}

GroupEntry::GroupEntry(AppsModel *parentModel, const QString &name, const QString &iconName, AbstractModel *childModel)
    : AbstractGroupEntry(parentModel)
    , m_name(name)
    , m_iconName(iconName)
    , m_childModel(childModel)
{
    QObject::connect(parentModel, &RootModel::cleared, childModel, &AbstractModel::deleteLater);

    QObject::connect(childModel, &AbstractModel::countChanged, [parentModel, this] {
        if (parentModel) {
            parentModel->entryChanged(this);
        }
    });
}

QString GroupEntry::name() const
{
    return m_name;
}

QString GroupEntry::icon() const
{
    return m_iconName;
}

bool GroupEntry::isNewlyInstalled() const
{
    if (m_childModel) {
        for (int i = 0; i < m_childModel->count(); ++i) {
            auto *entry = static_cast<AbstractEntry *>(m_childModel->index(i, 0).internalPointer());
            if (entry && entry->isNewlyInstalled()) {
                return true;
            }
        }
    }
    return false;
}

bool GroupEntry::hasChildren() const
{
    return m_childModel && m_childModel->count() > 0;
}

AbstractModel *GroupEntry::childModel() const
{
    return m_childModel;
}

AllAppsGroupEntry::AllAppsGroupEntry(AppsModel *parentModel, AbstractModel *childModel)
    : GroupEntry(parentModel, i18n("All Applications"), QStringLiteral("applications-all-symbolic"), childModel)
{
}

bool AllAppsGroupEntry::isNewlyInstalled() const
{
    // The highlight is for the user to find the app in the hierarchy,
    // there's no point in additionally highlighting the "all apps" category.
    return false;
}

RootModel::RootModel(QObject *parent)
    : AppsModel(QString(), parent)
    , m_favorites(new KAStatsFavoritesModel(this))
    , m_systemModel(nullptr)
    , m_showAllApps(false)
    , m_showAllAppsCategorized(false)
    , m_showRecentApps(true)
    , m_showRecentDocs(true)
    , m_recentOrdering(RecentUsageModel::Recent)
    , m_showPowerSession(true)
    , m_showFavoritesPlaceholder(false)
    , m_highlightNewlyInstalledApps(false)
    , m_refreshNewlyInstalledAppsTimer(nullptr)
    , m_recentAppsModel(nullptr)
    , m_recentDocsModel(nullptr)
{
}

RootModel::~RootModel()
{
}

QVariant RootModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entryList.count()) {
        return QVariant();
    }

    if (role == Kicker::HasActionListRole || role == Kicker::ActionListRole) {
        const AbstractEntry *entry = m_entryList.at(index.row());

        if (entry->type() == AbstractEntry::GroupType) {
            const auto *group = static_cast<const GroupEntry *>(entry);
            AbstractModel *model = group->childModel();

            if (model == m_recentAppsModel || model == m_recentDocsModel) {
                if (role == Kicker::HasActionListRole) {
                    return true;
                } else if (role == Kicker::ActionListRole) {
                    QVariantList actionList;
                    actionList << model->actions();
                    actionList << Kicker::createSeparatorActionItem();
                    actionList << Kicker::createActionItem(i18n("Hide %1", group->name()), QStringLiteral("view-hidden"), QStringLiteral("hideCategory"));
                    return actionList;
                }
            }
        }
    }

    return AppsModel::data(index, role);
}

bool RootModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    const AbstractEntry *entry = m_entryList.at(row);

    if (entry->type() == AbstractEntry::GroupType) {
        if (actionId == QLatin1String("hideCategory")) {
            AbstractModel *model = entry->childModel();

            if (model == m_recentAppsModel) {
                setShowRecentApps(false);

                return true;
            } else if (model == m_recentDocsModel) {
                setShowRecentDocs(false);

                return true;
            }
        } else if (entry->childModel()->hasActions()) {
            return entry->childModel()->trigger(-1, actionId, QVariant());
        }
    }

    return AppsModel::trigger(row, actionId, argument);
}

bool RootModel::showAllApps() const
{
    return m_showAllApps;
}

void RootModel::setShowAllApps(bool show)
{
    if (m_showAllApps != show) {
        m_showAllApps = show;

        refresh();

        Q_EMIT showAllAppsChanged();
    }
}

bool RootModel::showAllAppsCategorized() const
{
    return m_showAllAppsCategorized;
}

void RootModel::setShowAllAppsCategorized(bool showCategorized)
{
    if (m_showAllAppsCategorized != showCategorized) {
        m_showAllAppsCategorized = showCategorized;

        refresh();

        Q_EMIT showAllAppsCategorizedChanged();
    }
}

bool RootModel::showRecentApps() const
{
    return m_showRecentApps;
}

void RootModel::setShowRecentApps(bool show)
{
    if (show != m_showRecentApps) {
        m_showRecentApps = show;

        refresh();

        Q_EMIT showRecentAppsChanged();
    }
}

bool RootModel::showRecentDocs() const
{
    return m_showRecentDocs;
}

void RootModel::setShowRecentDocs(bool show)
{
    if (show != m_showRecentDocs) {
        m_showRecentDocs = show;

        refresh();

        Q_EMIT showRecentDocsChanged();
    }
}

int RootModel::recentOrdering() const
{
    return m_recentOrdering;
}

void RootModel::setRecentOrdering(int ordering)
{
    if (ordering != m_recentOrdering) {
        m_recentOrdering = ordering;

        refresh();

        Q_EMIT recentOrderingChanged();
    }
}

bool RootModel::showPowerSession() const
{
    return m_showPowerSession;
}

void RootModel::setShowPowerSession(bool show)
{
    if (show != m_showPowerSession) {
        m_showPowerSession = show;

        refresh();

        Q_EMIT showPowerSessionChanged();
    }
}

bool RootModel::showFavoritesPlaceholder() const
{
    return m_showFavoritesPlaceholder;
}

void RootModel::setShowFavoritesPlaceholder(bool show)
{
    if (show != m_showFavoritesPlaceholder) {
        m_showFavoritesPlaceholder = show;

        refresh();

        Q_EMIT showFavoritesPlaceholderChanged();
    }
}

bool RootModel::highlightNewlyInstalledApps() const
{
    return m_highlightNewlyInstalledApps;
}

void RootModel::setHighlightNewlyInstalledApps(bool highlight)
{
    if (highlight != m_highlightNewlyInstalledApps) {
        m_highlightNewlyInstalledApps = highlight;

        refresh();

        Q_EMIT highlightNewlyInstalledAppsChanged();
    }
}

AbstractModel *RootModel::favoritesModel()
{
    return m_favorites;
}

AbstractModel *RootModel::systemFavoritesModel()
{
    if (m_systemModel) {
        return m_systemModel->favoritesModel();
    }

    return nullptr;
}

void RootModel::refresh()
{
    if (!m_complete) {
        return;
    }

    beginResetModel();

    AppsModel::refreshInternal();

    AppsModel *allModel = nullptr;
    m_recentAppsModel = nullptr;
    m_recentDocsModel = nullptr;

    if (m_showAllApps) {
        QHash<QString, AbstractEntry *> appsHash;

        std::function<void(AbstractEntry *)> processEntry = [&](AbstractEntry *entry) {
            if (entry->type() == AbstractEntry::RunnableType) {
                auto *appEntry = static_cast<AppEntry *>(entry);
                appsHash.insert(appEntry->service()->menuId(), appEntry);
            } else if (entry->type() == AbstractEntry::GroupType) {
                auto *groupEntry = static_cast<GroupEntry *>(entry);
                AbstractModel *model = groupEntry->childModel();

                if (!model) {
                    return;
                }

                for (int i = 0; i < model->count(); ++i) {
                    processEntry(static_cast<AbstractEntry *>(model->index(i, 0).internalPointer()));
                }
            }
        };

        for (AbstractEntry *entry : std::as_const(m_entryList)) {
            processEntry(entry);
        }

        QList<AbstractEntry *> apps(appsHash.values());
        sortEntries(apps);

        if (!m_showAllAppsCategorized && !m_paginate) { // The app list built above goes into a model.
            allModel = new AppsModel(apps, false, this);
        } else if (m_paginate) { // We turn the apps list into a subtree of pages.
            m_favorites = new KAStatsFavoritesModel(this);
            Q_EMIT favoritesModelChanged();

            QList<AbstractEntry *> groups;

            int at = 0;
            QList<AbstractEntry *> page;
            page.reserve(m_pageSize);

            for (AbstractEntry *app : std::as_const(apps)) {
                page.append(app);

                if (at == (m_pageSize - 1)) {
                    at = 0;
                    auto *model = new AppsModel(page, false, this);
                    groups.append(new GroupEntry(this, QString(), QString(), model));
                    page.clear();
                } else {
                    ++at;
                }
            }

            if (!page.isEmpty()) {
                auto *model = new AppsModel(page, false, this);
                groups.append(new GroupEntry(this, QString(), QString(), model));
            }

            groups.prepend(new GroupEntry(this, QString(), QString(), m_favorites));

            allModel = new AppsModel(groups, true, this);
        } else { // We turn the apps list into a subtree of apps by starting letter.
            QList<AbstractEntry *> groups;
            QHash<QString, QList<AbstractEntry *>> m_categoryHash;

            for (const AbstractEntry *groupEntry : std::as_const(m_entryList)) {
                AbstractModel *model = groupEntry->childModel();

                if (!model)
                    continue;

                for (int i = 0; i < model->count(); ++i) {
                    AbstractEntry *appEntry = static_cast<AbstractEntry *>(model->index(i, 0).internalPointer());

                    // App entry's group stores a transliterated first character of the name. Prefer to use that.
                    QString name = appEntry->group();
                    if (name.isEmpty()) {
                        name = appEntry->name();
                    }

                    if (name.isEmpty()) {
                        continue;
                    }

                    const QChar &first = name.at(0).toUpper();
                    m_categoryHash[first.isDigit() ? QStringLiteral("0-9") : first].append(appEntry);
                }
            }

            QHashIterator<QString, QList<AbstractEntry *>> i(m_categoryHash);

            while (i.hasNext()) {
                i.next();
                auto *model = new AppsModel(i.value(), false, this);
                model->setDescription(i.key());
                groups.append(new GroupEntry(this, i.key(), QString(), model));
            }

            allModel = new AppsModel(groups, true, this);
        }

        allModel->setDescription(QStringLiteral("KICKER_ALL_MODEL")); // Intentionally no i18n.
    }

    // Whether we have any newly installed apps, or any that were recently uninstalled
    bool hasTrackedApp = false;

    auto stateConfig = Kicker::stateConfig();

    if (m_highlightNewlyInstalledApps) {
        // Track when we first see installed apps
        const QDate today = QDate::currentDate();

        QStringList installedApps;
        const QStringList storedInstalledApps = stateConfig->group(QString()).readEntry(QStringLiteral("InstalledApps"), QStringList());

        KConfigGroup applicationsGroup = stateConfig->group(QStringLiteral("Application"));

        std::function<void(AbstractEntry *)> processEntry = [&](AbstractEntry *entry) {
            if (entry->type() == AbstractEntry::RunnableType) {
                auto *appEntry = static_cast<AppEntry *>(entry);

                const QString appId = appEntry->id();
                installedApps.append(appId);

                QDate firstSeen;

                KConfigGroup group = applicationsGroup.group(appId);
                if (storedInstalledApps.isEmpty() || storedInstalledApps.contains(appId) || group.hasKey(QStringLiteral("LastSeen"))) {
                    // Initial survey with everything known, or already known to be installed, or recently uninstalled (in which case,
                    // if new, we held onto the FirstSeen key)
                    firstSeen = group.readEntry(QStringLiteral("FirstSeen"), QDate());
                } else if (!storedInstalledApps.contains(appId)) {
                    qCDebug(KICKER_DEBUG) << appId << "appears to be newly installed";
                    group.writeEntry(QStringLiteral("FirstSeen"), today);
                    firstSeen = today;
                }

                appEntry->setFirstSeen(firstSeen);
                if (appEntry->isNewlyInstalled()) {
                    hasTrackedApp = true;
                } else {
                    applicationsGroup.deleteGroup(appId);
                }
            } else if (entry->type() == AbstractEntry::GroupType) {
                auto *groupEntry = static_cast<GroupEntry *>(entry);
                if (AbstractModel *model = groupEntry->childModel()) {
                    for (int i = 0; i < model->count(); ++i) {
                        processEntry(static_cast<AbstractEntry *>(model->index(i, 0).internalPointer()));
                    }
                }
            }
        };

        for (AbstractEntry *entry : std::as_const(m_entryList)) {
            processEntry(entry);
        }

        // Remember uninstalled app (including, if new, when it was first seen)
        for (const QString &appId : storedInstalledApps) {
            if (!installedApps.contains(appId)) {
                // App was uninstalled
                qCDebug(KICKER_DEBUG) << appId << "is being remembered after being uninstalled";
                KConfigGroup group = applicationsGroup.group(appId);
                group.writeEntry(QStringLiteral("LastSeen"), today);
                hasTrackedApp = true;
            }
        }

        // Stop remembering uninstalled apps after 3 days
        for (const QString &appId : applicationsGroup.groupList()) {
            if (!installedApps.contains(appId)) {
                KConfigGroup group = applicationsGroup.group(appId);
                const QDate lastSeen = group.readEntry(QStringLiteral("LastSeen"), QDate());
                if (lastSeen.isValid() && lastSeen.daysTo(QDate::currentDate()) < s_rememberUninstalledDays) {
                    hasTrackedApp = true;
                } else {
                    qCDebug(KICKER_DEBUG) << appId << "is no longer being remembered after being uninstalled";
                    applicationsGroup.deleteGroup(appId);
                }
            }
        }

        stateConfig->group(QString()).writeEntry(QStringLiteral("InstalledApps"), installedApps);
    }

    trackNewlyInstalledApps(hasTrackedApp);

    int separatorPosition = 0;

    if (allModel) {
        m_entryList.prepend(new AllAppsGroupEntry(this, allModel));
        ++separatorPosition;
    }

    if (m_showFavoritesPlaceholder) {
        // This entry is a placeholder and shouldn't ever be visible
        QList<AbstractEntry *> placeholderList;
        auto *placeholderModel = new AppsModel(placeholderList, false, this);

        // Favorites group containing a placeholder entry, so it would be considered as a group, not an entry
        QList<AbstractEntry *> placeholderEntry;
        placeholderEntry.append(new GroupEntry(this, //
                                               i18n("This shouldn't be visible! Use KICKER_FAVORITES_MODEL"),
                                               QStringLiteral("dialog-warning"),
                                               placeholderModel));
        auto *favoritesPlaceholderModel = new AppsModel(placeholderEntry, false, this);

        favoritesPlaceholderModel->setDescription(QStringLiteral("KICKER_FAVORITES_MODEL")); // Intentionally no i18n.
        m_entryList.prepend(new GroupEntry(this, i18n("Favorites"), QStringLiteral("bookmarks"), favoritesPlaceholderModel));
        ++separatorPosition;
    }

    if (m_showRecentDocs) {
        m_recentDocsModel = new RecentUsageModel(this, RecentUsageModel::OnlyDocs, m_recentOrdering);
        m_entryList.prepend(new GroupEntry(this,
                                           m_recentOrdering == RecentUsageModel::Recent ? i18n("Recent Files") : i18n("Often Used Files"),
                                           m_recentOrdering == RecentUsageModel::Recent ? QStringLiteral("view-history") : QStringLiteral("office-chart-pie"),
                                           m_recentDocsModel));
        ++separatorPosition;
    }

    if (m_showRecentApps) {
        m_recentAppsModel = new RecentUsageModel(this, RecentUsageModel::OnlyApps, m_recentOrdering);
        m_entryList.prepend(new GroupEntry(this,
                                           m_recentOrdering == RecentUsageModel::Recent ? i18n("Recent Applications") : i18n("Often Used Applications"),
                                           m_recentOrdering == RecentUsageModel::Recent ? QStringLiteral("view-history") : QStringLiteral("office-chart-pie"),
                                           m_recentAppsModel));
        ++separatorPosition;
    }

    if (m_showSeparators && separatorPosition > 0) {
        m_entryList.insert(separatorPosition, new SeparatorEntry(this));
        ++m_separatorCount;
    }

    m_systemModel = new SystemModel(this);
    QObject::connect(m_systemModel, &SystemModel::sessionManagementStateChanged, this, &RootModel::refresh);

    if (m_showPowerSession) {
        m_entryList << new GroupEntry(this, i18n("Power / Session"), QStringLiteral("system-log-out"), m_systemModel);
    }

    endResetModel();

    m_favorites->refresh();

    Q_EMIT systemFavoritesModelChanged();
    Q_EMIT countChanged();
    Q_EMIT separatorCountChanged();

    Q_EMIT refreshed();
}

void RootModel::trackNewlyInstalledApps(const bool track)
{
    const bool isTracking = m_refreshNewlyInstalledAppsTimer && m_refreshNewlyInstalledAppsTimer->isActive();

    if (track == isTracking) {
        return;
    }

    if (track) {
        // Begin tracking newly (un)installed apps
        if (!m_refreshNewlyInstalledAppsTimer) {
            m_refreshNewlyInstalledAppsTimer = new QTimer(this);
            m_refreshNewlyInstalledAppsTimer->setInterval(24h);
            m_refreshNewlyInstalledAppsTimer->callOnTimeout(this, &RootModel::refreshNewlyInstalledApps);
        }
        if (!m_refreshNewlyInstalledAppsTimer->isActive()) {
            qCDebug(KICKER_DEBUG) << "Starting periodic newly installed apps check";
            m_refreshNewlyInstalledAppsTimer->start();
        }

        QDBusConnection::sessionBus().connect(QStringLiteral("org.kde.ActivityManager"),
                                              QStringLiteral("/ActivityManager/Resources/Scoring"),
                                              QStringLiteral("org.kde.ActivityManager.ResourcesScoring"),
                                              QStringLiteral("ResourceScoreUpdated"),
                                              this,
                                              SLOT(onResourceScoresChanged(QString, QString, QString, double, unsigned int, unsigned int)));
    } else {
        // Stop tracking newly (un)installed apps
        if (m_refreshNewlyInstalledAppsTimer) {
            qCDebug(KICKER_DEBUG) << "Stopping periodic newly installed apps check";
            m_refreshNewlyInstalledAppsTimer->stop();
        }

        QDBusConnection::sessionBus().disconnect(QStringLiteral("org.kde.ActivityManager"),
                                                 QStringLiteral("/ActivityManager/Resources/Scoring"),
                                                 QStringLiteral("org.kde.ActivityManager.ResourcesScoring"),
                                                 QStringLiteral("ResourceScoreUpdated"),
                                                 this,
                                                 SLOT(onResourceScoresChanged(QString, QString, QString, double, unsigned int, unsigned int)));
    }
}

void RootModel::refreshNewlyInstalledApps()
{
    qCDebug(KICKER_DEBUG) << "Refreshing newly installed apps";
    Q_ASSERT(m_highlightNewlyInstalledApps);

    QStringList installedApps;

    KSharedConfig::Ptr stateConfig = Kicker::stateConfig();
    KConfigGroup applicationsGroup = stateConfig->group(QStringLiteral("Application"));

    bool hasTrackedApp = false;

    std::function<void(AbstractEntry *)> processEntry = [&](AbstractEntry *entry) {
        if (entry->type() == AbstractEntry::RunnableType) {
            auto *appEntry = static_cast<AppEntry *>(entry);

            const QString appId = appEntry->id();
            installedApps.append(appId);

            if (appEntry->isNewlyInstalled()) {
                hasTrackedApp = true;
            } else if (appEntry->firstSeen().isValid()) {
                qCDebug(KICKER_DEBUG) << appEntry->id() << "is no longer considered newly installed";
                appEntry->setFirstSeen(QDate());
                applicationsGroup.deleteGroup(appEntry->id());

                refreshNewlyInstalledEntry(appEntry);
            }
        } else if (entry->type() == AbstractEntry::GroupType) {
            auto *groupEntry = static_cast<GroupEntry *>(entry);
            if (AbstractModel *model = groupEntry->childModel()) {
                for (int i = 0; i < model->count(); ++i) {
                    if (auto *entry = static_cast<AbstractEntry *>(model->index(i, 0).internalPointer())) {
                        processEntry(entry);
                    }
                }
            }
        }
    };

    for (AbstractEntry *entry : std::as_const(m_entryList)) {
        processEntry(entry);
    }

    // Stop remembering uninstalled apps after 3 days
    for (const QString &appId : applicationsGroup.groupList()) {
        if (!installedApps.contains(appId)) {
            KConfigGroup group = applicationsGroup.group(appId);
            const QDate lastSeen = group.readEntry(QStringLiteral("LastSeen"), QDate());
            if (lastSeen.isValid() && lastSeen.daysTo(QDate::currentDate()) < s_rememberUninstalledDays) {
                hasTrackedApp = true;
            } else {
                qCDebug(KICKER_DEBUG) << appId << "is no longer being remembered after being uninstalled";
                applicationsGroup.deleteGroup(appId);
            }
        }
    }

    // Stop tracking when there's nothing left to track
    trackNewlyInstalledApps(hasTrackedApp);
}

void RootModel::onResourceScoresChanged(const QString &activity,
                                        const QString &client,
                                        const QString &resource,
                                        double score,
                                        unsigned int lastUpdate,
                                        unsigned int firstUpdate)
{
    Q_UNUSED(activity);
    Q_UNUSED(client);
    Q_UNUSED(score);
    Q_UNUSED(lastUpdate);
    Q_UNUSED(firstUpdate);

    constexpr QLatin1String s_prefix("applications:");
    if (!resource.startsWith(s_prefix)) {
        return;
    }

    const QStringView appId = QStringView(resource).mid(s_prefix.size());

    std::function<void(AbstractEntry *)> processEntry = [&](AbstractEntry *entry) {
        if (entry->type() == AbstractEntry::RunnableType) {
            auto *appEntry = static_cast<AppEntry *>(entry);

            if (appEntry->id() == appId && appEntry->isNewlyInstalled()) {
                qCDebug(KICKER_DEBUG) << appEntry->id() << "is no longer considered newly installed (resourceScore)";
                appEntry->setFirstSeen(QDate());
                auto stateConfig = Kicker::stateConfig();
                KConfigGroup applicationsGroup = stateConfig->group(QStringLiteral("Application"));
                applicationsGroup.deleteGroup(appEntry->id());

                refreshNewlyInstalledEntry(appEntry);
            }
        } else if (entry->type() == AbstractEntry::GroupType) {
            auto *groupEntry = static_cast<GroupEntry *>(entry);
            if (AbstractModel *model = groupEntry->childModel()) {
                for (int i = 0; i < model->count(); ++i) {
                    processEntry(static_cast<AbstractEntry *>(model->index(i, 0).internalPointer()));
                }
            }
        }
    };

    for (AbstractEntry *entry : std::as_const(m_entryList)) {
        processEntry(entry);
    }
}

#include "moc_rootmodel.cpp"
