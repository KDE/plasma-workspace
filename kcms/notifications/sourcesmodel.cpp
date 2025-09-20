/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "sourcesmodel.h"
#include "notifyrcpaths.h"

#include <QCollator>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>

#include <KApplicationTrader>
#include <KConfig>
#include <KConfigGroup>
#include <KFileUtils>
#include <KLocalizedString>
#include <KSharedConfig>

#include <algorithm>

using namespace Qt::StringLiterals;

static const QRegularExpression s_eventGroupRegExp(QStringLiteral("^Event/([^/]*)$"));

SourcesModel::SourcesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

SourcesModel::~SourcesModel() = default;

QPersistentModelIndex SourcesModel::makePersistentModelIndex(const QModelIndex &idx) const
{
    return QPersistentModelIndex(idx);
}

QPersistentModelIndex SourcesModel::persistentIndexForDesktopEntry(const QString &desktopEntry) const
{
    if (desktopEntry.isEmpty()) {
        return QPersistentModelIndex();
    }
    const auto matches = match(index(0, 0), SourcesModel::DesktopEntryRole, desktopEntry, 1, Qt::MatchFixedString);
    if (matches.isEmpty()) {
        return QPersistentModelIndex();
    }
    return QPersistentModelIndex(matches.first());
}

QPersistentModelIndex SourcesModel::persistentIndexForNotifyRcName(const QString &notifyRcName) const
{
    if (notifyRcName.isEmpty()) {
        return QPersistentModelIndex();
    }
    const auto matches = match(index(0, 0), SourcesModel::NotifyRcNameRole, notifyRcName, 1, Qt::MatchFixedString);
    if (matches.isEmpty()) {
        return QPersistentModelIndex();
    }
    return QPersistentModelIndex(matches.first());
}

QPersistentModelIndex SourcesModel::indexOfEvent(const QModelIndex &parent, const QString &eventId) const
{
    if (!checkIndex(parent, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid) || !hasChildren(parent)) {
        return QPersistentModelIndex();
    }

    const auto matches = match(index(0, 0, parent), EventIdRole, eventId, 1, Qt::MatchFixedString);
    return matches.isEmpty() ? QPersistentModelIndex() : QPersistentModelIndex(matches.first());
}

int SourcesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

int SourcesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        return m_data.count();
    }

    if (parent.internalId()) {
        return 0;
    }

    return m_data.at(parent.row()).events.count();
}

QVariant SourcesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.internalId()) { // event
        const auto &events = m_data.at(index.internalId() - 1).events;
        const auto event = events.at(index.row());

        switch (role) {
        case Qt::DisplayRole:
            return event->name();
        case Qt::DecorationRole:
            return event->iconName();
        case EventIdRole:
            return s_eventGroupRegExp.match(event->currentGroup()).captured(1);
        case CommentRole:
            return event->comment();
        case ActionsRole:
            return event->action().split(QLatin1Char('|'), Qt::SkipEmptyParts);
        case SoundRole:
            return event->sound();
        case DefaultActionsRole: {
            // Weird KConfigSkeleton API to get the cascaded default values
            event->useDefaults(true);
            const QStringList defaultActions = event->action().split(QLatin1Char('|'), Qt::SkipEmptyParts);
            event->useDefaults(false);
            return defaultActions;
        }
        case DefaultSoundRole: {
            // Weird KConfigSkeleton API to get the cascaded default values
            event->useDefaults(true);
            const QString defaultSound = event->sound();
            event->useDefaults(false);
            return defaultSound;
        }
        case IsDefaultRole:
            return event->isDefaults();
        case ShowIconsRole:
            // We show the icons when at least one of the events specifies an icon name
            return std::any_of(events.cbegin(), events.cend(), [](auto *event) {
                return !event->iconName().isEmpty();
            });
        }

        return QVariant();
    }

    const auto &source = m_data.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return source.display();
    case Qt::DecorationRole:
        return source.iconName;
    case SourceTypeRole:
        return source.desktopEntry.isEmpty() ? ServiceType : ApplicationType;
    case NotifyRcNameRole:
        return source.notifyRcName;
    case DesktopEntryRole:
        return source.desktopEntry;
    case IsDefaultRole:
        return source.isDefault && std::all_of(source.events.cbegin(), source.events.cend(), [](auto event) {
                   return event->isDefaults();
               });
    }

    return QVariant();
}

bool SourcesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (!index.internalId()) { // source
        auto &source = m_data[index.row()];

        switch (role) {
        case IsDefaultRole: {
            if (source.isDefault != value.toBool()) {
                source.isDefault = value.toBool();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            break;
        }
        }
        return false;
    }

    NotificationManager::EventSettings *event = m_data[index.internalId() - 1].events[index.row()];

    const bool wasDefault = event->isDefaults();
    QList<int> changedRoles;

    switch (role) {
    case ActionsRole: {
        const QString newAction = value.toStringList().join(QLatin1Char('|'));
        if (event->action() != newAction) {
            event->setAction(newAction);
            changedRoles << role;
        }
        break;
    }
    case SoundRole: {
        const QString newSound = value.toString();
        if (event->sound() != newSound) {
            event->setSound(newSound);
            changedRoles << role;
        }
        break;
    }
    }

    if (event->isDefaults() != wasDefault) {
        changedRoles << IsDefaultRole;
    }

    if (changedRoles.isEmpty()) {
        return false;
    }

    Q_EMIT dataChanged(index, index, changedRoles);
    // Also notify the possible defaults change in the parent source index
    if (changedRoles.contains(IsDefaultRole)) {
        const QModelIndex sourceIndex = this->index(index.internalId() - 1, 0, QModelIndex());
        Q_EMIT dataChanged(sourceIndex, sourceIndex, {IsDefaultRole});
    }
    return true;
}

QModelIndex SourcesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    if (parent.isValid()) {
        const auto events = m_data.at(parent.row()).events;
        if (row < events.count()) {
            return createIndex(row, column, parent.row() + 1);
        }

        return QModelIndex();
    }

    if (row < m_data.count()) {
        return createIndex(row, column, nullptr);
    }

    return QModelIndex();
}

QModelIndex SourcesModel::parent(const QModelIndex &child) const
{
    if (child.internalId()) {
        return createIndex(child.internalId() - 1, 0, nullptr);
    }

    return QModelIndex();
}

QHash<int, QByteArray> SourcesModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {Qt::DecorationRole, QByteArrayLiteral("decoration")},
        {SourceTypeRole, QByteArrayLiteral("sourceType")},
        {NotifyRcNameRole, QByteArrayLiteral("notifyRcName")},
        {DesktopEntryRole, QByteArrayLiteral("desktopEntry")},
        {IsDefaultRole, QByteArrayLiteral("isDefault")},
        {EventIdRole, QByteArrayLiteral("eventId")},
        {CommentRole, QByteArrayLiteral("comment")},
        {ShowIconsRole, QByteArrayLiteral("showIcons")},
        {ActionsRole, QByteArrayLiteral("actions")},
        {SoundRole, QByteArrayLiteral("sound")},
        {DefaultActionsRole, QByteArrayLiteral("defaultActions")},
        {DefaultSoundRole, QByteArrayLiteral("defaultSound")},
    };
}

void SourcesModel::load()
{
    beginResetModel();

    m_data.clear();

    QCollator collator;

    QList<SourceData> appsData;
    QList<SourceData> servicesData;

    QStringList desktopEntries;

    const QStringList filePaths = KFileUtils::findAllUniqueFiles(NotifyRcPaths::allSearchPaths(), {QStringLiteral("*.notifyrc")});
    for (const QString &filePath : filePaths) {
        const QFileInfo fileInfo(filePath);
        const QString fileName = fileInfo.fileName();
        const QString dirName = fileInfo.dir().dirName(); // "knotifications6" or "knotifications5"

        KSharedConfig::Ptr config = KSharedConfig::openConfig(fileName, KConfig::NoGlobals);
        // `QStandardPaths` follows the order of precedence given by `$XDG_DATA_DIRS
        // (more priority goest first), but for `addConfigSources() it is the opposite
        QStringList configSources = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("%2/%1").arg(fileName).arg(dirName));
        std::reverse(configSources.begin(), configSources.end());
        // if this didn't find anything, it's a notifyrc file from a Flatpak
        config->addConfigSources(configSources.isEmpty() ? QStringList{filePath} : configSources);

        KConfigGroup globalGroup(config, QLatin1String("Global"));

        const QString desktopEntry = globalGroup.readEntry(QStringLiteral("DesktopEntry"));
        if (!desktopEntry.isEmpty()) {
            if (desktopEntries.contains(desktopEntry)) {
                continue;
            }
            desktopEntries.append(desktopEntry);
        }

        SourceData source{
            // The old KCM read the Name and Comment from global settings disregarding
            // any user settings and just used user-specific files for actions config
            // I'm pretty sure there's a readEntry equivalent that does that without
            // reading the config stuff twice, assuming we care about this to begin with
            .name = globalGroup.readEntry(QStringLiteral("Name")),
            .comment = globalGroup.readEntry(QStringLiteral("Comment")),
            .iconName = globalGroup.readEntry(QStringLiteral("IconName")),
            .isDefault = true,
            .notifyRcName = fileName.section(QLatin1Char('.'), 0, -2),
            .desktopEntry = desktopEntry,
            .events = {},
        };

        // Add events
        const QStringList groups = config->groupList().filter(s_eventGroupRegExp);

        QList<NotificationManager::EventSettings *> events;
        events.reserve(groups.size());
        for (const QString &group : groups) {
            const QString eventId = s_eventGroupRegExp.match(group).captured(1);
            events.append(new NotificationManager::EventSettings(config, eventId, this));
        }
        std::sort(events.begin(), events.end(), [&collator](NotificationManager::EventSettings *a, NotificationManager::EventSettings *b) {
            return collator.compare(a->name(), b->name()) < 0;
        });
        source.events = events;

        if (!source.desktopEntry.isEmpty()) {
            appsData.append(source);
        } else {
            servicesData.append(source);
        }
    }

    const auto services = KApplicationTrader::query([desktopEntries](const KService::Ptr &app) {
        if (app->noDisplay()) {
            return false;
        }

        if (desktopEntries.contains(app->desktopEntryName())) {
            return false;
        }

        if (!app->property<bool>(QStringLiteral("X-GNOME-UsesNotifications"))) {
            return false;
        }

        return true;
    });

    for (const auto &service : services) {
        appsData.append(SourceData::fromService(service));
        desktopEntries.append(service->desktopEntryName());
    }

    KSharedConfig::Ptr plasmanotifyrc = KSharedConfig::openConfig(u"plasmanotifyrc"_s);
    KConfigGroup applicationsGroup = plasmanotifyrc->group(u"Applications"_s);
    const QStringList seenApps = applicationsGroup.groupList();
    for (const QString &app : seenApps) {
        if (desktopEntries.contains(app)) {
            continue;
        }

        KService::Ptr service = KService::serviceByDesktopName(app);
        if (!service || service->noDisplay()) {
            continue;
        }

        appsData.append(SourceData::fromService(service));
        desktopEntries.append(service->desktopEntryName());
    }

    std::sort(appsData.begin(), appsData.end(), [&collator](const SourceData &a, const SourceData &b) {
        return collator.compare(a.display(), b.display()) < 0;
    });

    // Fake entry for configuring non-identifyable applications
    appsData << SourceData{
        .name = i18n("Other Applications"),
        .comment = {},
        .iconName = QStringLiteral("applications-other"),
        .isDefault = true,
        .notifyRcName = {},
        .desktopEntry = QStringLiteral("@other"),
        .events = {},
    };

    std::sort(servicesData.begin(), servicesData.end(), [&collator](const SourceData &a, const SourceData &b) {
        return collator.compare(a.display(), b.display()) < 0;
    });

    m_data << appsData << servicesData;

    endResetModel();
}

void SourcesModel::loadEvents()
{
    beginResetModel();

    for (const SourceData &source : std::as_const(m_data)) {
        for (auto &event : source.events) {
            event->load();
        }
    }

    endResetModel();
}

void SourcesModel::saveEvents()
{
    for (const SourceData &source : std::as_const(m_data)) {
        for (auto &event : source.events) {
            event->save();
        }

        if (!source.notifyRcName.isEmpty()) {
            QDBusMessage message =
                QDBusMessage::createSignal(QStringLiteral("/Config"), QStringLiteral("org.kde.knotification"), QStringLiteral("reparseConfiguration"));
            message.setArguments({source.notifyRcName});
            QDBusConnection::sessionBus().send(message);
        }
    }
}

bool SourcesModel::isEventDefaults() const
{
    for (const SourceData &source : std::as_const(m_data)) {
        for (const auto &event : source.events) {
            if (!event->isDefaults()) {
                return false;
            }
        }
    }
    return true;
}

bool SourcesModel::isEventSaveNeeded() const
{
    for (const SourceData &source : std::as_const(m_data)) {
        for (const auto &event : source.events) {
            if (event->isSaveNeeded()) {
                return true;
            }
        }
    }
    return false;
}

void SourcesModel::setEventDefaults()
{
    beginResetModel();

    for (const SourceData &source : std::as_const(m_data)) {
        for (auto &event : source.events) {
            event->setDefaults();
        }
    }

    endResetModel();
}

SourceData SourceData::fromService(KService::Ptr service)
{
    return SourceData{
        .name = service->name(),
        .comment = service->comment(),
        .iconName = service->icon(),
        .isDefault = true,
        .notifyRcName = {},
        .desktopEntry = service->desktopEntryName(),
        .events = {},
    };
}

#include "moc_sourcesmodel.cpp"
