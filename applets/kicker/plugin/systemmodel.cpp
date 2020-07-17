/***************************************************************************
 *   Copyright (C) 2014-2015 by Eike Hein <hein@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "systemmodel.h"
#include "actionlist.h"
#include "simplefavoritesmodel.h"

#include <QStandardPaths>

#include <KDirWatch>
#include <KLocalizedString>

SystemModel::SystemModel(QObject *parent) : AbstractModel(parent)
{
    m_favoritesModel = new SimpleFavoritesModel(this);

    m_entries[SystemEntry::LockSession] = new SystemEntry(this, SystemEntry::LockSession);
    m_entries[SystemEntry::LogoutSession] = new SystemEntry(this, SystemEntry::LogoutSession);
    m_entries[SystemEntry::SaveSession] = new SystemEntry(this, SystemEntry::SaveSession);
    m_entries[SystemEntry::SwitchUser] = new SystemEntry(this, SystemEntry::SwitchUser);
    m_entries[SystemEntry::Suspend] = new SystemEntry(this, SystemEntry::Suspend);
    m_entries[SystemEntry::Hibernate] = new SystemEntry(this, SystemEntry::Hibernate);
    m_entries[SystemEntry::Reboot] = new SystemEntry(this, SystemEntry::Reboot);
    m_entries[SystemEntry::Shutdown] = new SystemEntry(this, SystemEntry::Shutdown);

    for (SystemEntry *entry : m_entries.values()) {
        QObject::connect(entry, &SystemEntry::isValidChanged, this,
            [this, entry]() {
                const QModelIndex &idx = index(entry->action(), 0);
                emit dataChanged(idx, idx, QVector<int>{Kicker::DisabledRole});
            }
        );

        QObject::connect(entry, &SystemEntry::isValidChanged, m_favoritesModel, &AbstractModel::refresh);
    }
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

    const SystemEntry *entry = m_entries.value(static_cast<SystemEntry::Action>(index.row() + 1));

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
        m_entries.value(static_cast<SystemEntry::Action>(row + 1))->run(actionId, argument);

        return true;
    }

    return false;
}

void SystemModel::refresh()
{
    m_favoritesModel->refresh();
}
