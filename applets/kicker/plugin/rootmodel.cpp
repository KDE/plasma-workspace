/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "rootmodel.h"
#include "actionlist.h"
#include "kastatsfavoritesmodel.h"
#include "recentcontactsmodel.h"
#include "recentusagemodel.h"
#include "systemmodel.h"

#include <KLocalizedString>

#include <QCollator>

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

QIcon GroupEntry::icon() const
{
    return QIcon::fromTheme(m_iconName, QIcon::fromTheme(QStringLiteral("unknown")));
}

bool GroupEntry::hasChildren() const
{
    return m_childModel && m_childModel->count() > 0;
}

AbstractModel *GroupEntry::childModel() const
{
    return m_childModel;
}

RootModel::RootModel(QObject *parent)
    : AppsModel(QString(), parent)
    , m_favorites(new KAStatsFavoritesModel(this))
    , m_systemModel(nullptr)
    , m_showAllApps(false)
    , m_showAllAppsCategorized(false)
    , m_showRecentApps(true)
    , m_showRecentDocs(true)
    , m_showRecentContacts(false)
    , m_recentOrdering(RecentUsageModel::Recent)
    , m_showPowerSession(true)
    , m_showFavoritesPlaceholder(false)
    , m_recentAppsModel(nullptr)
    , m_recentDocsModel(nullptr)
    , m_recentContactsModel(nullptr)
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
            const GroupEntry *group = static_cast<const GroupEntry *>(entry);
            AbstractModel *model = group->childModel();

            if (model == m_recentAppsModel || model == m_recentDocsModel || model == m_recentContactsModel) {
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
            } else if (model == m_recentContactsModel) {
                setShowRecentContacts(false);

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

bool RootModel::showRecentContacts() const
{
    return m_showRecentContacts;
}

void RootModel::setShowRecentContacts(bool show)
{
    if (show != m_showRecentContacts) {
        m_showRecentContacts = show;

        refresh();

        Q_EMIT showRecentContactsChanged();
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
    m_recentContactsModel = nullptr;

    if (m_showAllApps) {
        QHash<QString, AbstractEntry *> appsHash;

        std::function<void(AbstractEntry *)> processEntry = [&](AbstractEntry *entry) {
            if (entry->type() == AbstractEntry::RunnableType) {
                AppEntry *appEntry = static_cast<AppEntry *>(entry);
                appsHash.insert(appEntry->service()->menuId(), appEntry);
            } else if (entry->type() == AbstractEntry::GroupType) {
                GroupEntry *groupEntry = static_cast<GroupEntry *>(entry);
                AbstractModel *model = groupEntry->childModel();

                if (!model) {
                    return;
                }

                for (int i = 0; i < model->count(); ++i) {
                    processEntry(static_cast<AbstractEntry *>(model->index(i, 0).internalPointer()));
                }
            }
        };

        for (AbstractEntry *entry : qAsConst(m_entryList)) {
            processEntry(entry);
        }

        QList<AbstractEntry *> apps(appsHash.values());
        QCollator c;

        std::sort(apps.begin(), apps.end(), [&c](AbstractEntry *a, AbstractEntry *b) {
            if (a->type() != b->type()) {
                return a->type() > b->type();
            } else {
                return c.compare(a->name(), b->name()) < 0;
            }
        });

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
                    AppsModel *model = new AppsModel(page, false, this);
                    groups.append(new GroupEntry(this, QString(), QString(), model));
                    page.clear();
                } else {
                    ++at;
                }
            }

            if (!page.isEmpty()) {
                AppsModel *model = new AppsModel(page, false, this);
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

                    if (appEntry->name().isEmpty()) {
                        continue;
                    }

                    const QChar &first = appEntry->name().at(0).toUpper();
                    m_categoryHash[first.isDigit() ? QStringLiteral("0-9") : first].append(appEntry);
                }
            }

            QHashIterator<QString, QList<AbstractEntry *>> i(m_categoryHash);

            while (i.hasNext()) {
                i.next();
                AppsModel *model = new AppsModel(i.value(), false, this);
                model->setDescription(i.key());
                groups.append(new GroupEntry(this, i.key(), QString(), model));
            }

            allModel = new AppsModel(groups, true, this);
        }

        allModel->setDescription(QStringLiteral("KICKER_ALL_MODEL")); // Intentionally no i18n.
    }

    int separatorPosition = 0;

    if (allModel) {
        m_entryList.prepend(new GroupEntry(this, i18n("All Applications"), QStringLiteral("applications-all"), allModel));
        ++separatorPosition;
    }

    if (m_showFavoritesPlaceholder) {
        // This entry is a placeholder and shouldn't ever be visible
        QList<AbstractEntry *> placeholderList;
        AppsModel *placeholderModel = new AppsModel(placeholderList, false, this);

        // Favorites group containing a placeholder entry, so it would be considered as a group, not an entry
        QList<AbstractEntry *> placeholderEntry;
        placeholderEntry.append(new GroupEntry(this, //
                                               i18n("This shouldn't be visible! Use KICKER_FAVORITES_MODEL"),
                                               QStringLiteral("dialog-warning"),
                                               placeholderModel));
        AppsModel *favoritesPlaceholderModel = new AppsModel(placeholderEntry, false, this);

        favoritesPlaceholderModel->setDescription(QStringLiteral("KICKER_FAVORITES_MODEL")); // Intentionally no i18n.
        m_entryList.prepend(new GroupEntry(this, i18n("Favorites"), QStringLiteral("bookmarks"), favoritesPlaceholderModel));
        ++separatorPosition;
    }

    if (m_showRecentContacts) {
        m_recentContactsModel = new RecentContactsModel(this);
        m_entryList.prepend(new GroupEntry(this, i18n("Recent Contacts"), QStringLiteral("view-history"), m_recentContactsModel));
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
