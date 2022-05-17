/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemmodel.h"
#include "actionlist.h"
#include "simplefavoritesmodel.h"

#include <QStandardPaths>

#include <KDirWatch>
#include <KLocalizedString>

SystemModel::SystemModel(QObject *parent)
    : AbstractModel(parent)
{
    m_favoritesModel = new SimpleFavoritesModel(this);

    populate();
}

SystemModel::~SystemModel()
{
    qDeleteAll(m_entries);
}

QString SystemModel::description() const
{
    return i18n("System actions");
}

QVariant SystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.count()) {
        return QVariant();
    }

    const SystemEntry *entry = m_entries.value(index.row());

    if (role == Qt::DisplayRole) {
        return entry->name();
    } else if (role == Qt::DecorationRole) {
        return entry->iconName();
    } else if (role == Kicker::DescriptionRole) {
        return entry->description();
    } else if (role == Kicker::GroupRole) {
        return entry->group();
    } else if (role == Kicker::FavoriteIdRole) {
        return entry->id();
    } else if (role == Kicker::HasActionListRole) {
        return entry->hasActions();
    } else if (role == Kicker::ActionListRole) {
        return entry->actions();
    } else if (role == Kicker::DisabledRole) {
        return !entry->isValid();
    }

    return QVariant();
}

int SystemModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.count();
}

bool SystemModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    if (row >= 0 && row < m_entries.count()) {
        m_entries.at(row)->run(actionId, argument);

        return true;
    }

    return false;
}

void SystemModel::refresh()
{
    beginResetModel();
    populate();
    endResetModel();

    m_favoritesModel->refresh();
}

void SystemModel::populate()
{
    qDeleteAll(m_entries);
    qDeleteAll(m_invalidEntries);
    m_entries.clear();
    m_invalidEntries.clear();

    auto addIfValid = [=](const SystemEntry::Action action) {
        SystemEntry *entry = new SystemEntry(this, action);
        QObject::connect(entry, &SystemEntry::sessionManagementStateChanged, this, &SystemModel::sessionManagementStateChanged);

        if (entry->isValid()) {
            m_entries << entry;
        } else {
            m_invalidEntries << entry;
        }

        QObject::connect(entry, &SystemEntry::isValidChanged, this, &AbstractModel::refresh, Qt::UniqueConnection);
    };

    addIfValid(SystemEntry::LockSession);
    addIfValid(SystemEntry::LogoutSession);
    addIfValid(SystemEntry::SaveSession);
    addIfValid(SystemEntry::SwitchUser);
    addIfValid(SystemEntry::Suspend);
    addIfValid(SystemEntry::Hibernate);
    addIfValid(SystemEntry::Reboot);
    addIfValid(SystemEntry::Shutdown);
}
