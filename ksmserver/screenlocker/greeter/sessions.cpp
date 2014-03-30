/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "sessions.h"
// workspace
#include <kdisplaymanager.h>
// KDE
#include <KAuthorized>
#include <KLocalizedString>

namespace ScreenLocker
{

UserSessionsModel::UserSessionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    init();
    QHash<int, QByteArray> roles;
    roles[Qt::UserRole] = "session";
    roles[Qt::UserRole + 1] = "location";
    roles[Qt::UserRole + 2] = "vt";
    setRoleNames(roles);
}

UserSessionsModel::~UserSessionsModel()
{
}

QVariant UserSessionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (index.row() < 0 || index.row() >= m_model.size()) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole:
    case Qt::UserRole:
        return m_model[index.row()].m_session;
    case (Qt::UserRole + 1):
        return m_model[index.row()].m_location;
    case (Qt::UserRole + 2):
        return m_model[index.row()].m_vt;
    default:
        return QVariant();
    }
}

QVariant UserSessionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::UserRole:
        return i18n("Session");
    case Qt::UserRole + 1:
        return i18n("Location");
    default:
        return QAbstractItemModel::headerData(section, orientation, role);
    }
}

int UserSessionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_model.count();
}

Qt::ItemFlags UserSessionsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QAbstractItemModel::flags(index);
    }
    if (!m_model[index.row()].m_enabled) {
        return QAbstractItemModel::flags(index) & ~Qt::ItemIsEnabled;
    }
    return QAbstractItemModel::flags(index);
}

void UserSessionsModel::init()
{
    beginResetModel();
    m_model.clear();
    KDisplayManager dm;

    SessList sess;
    if (dm.localSessions(sess)) {

        QString user, loc;
        for (SessList::ConstIterator it = sess.constBegin(); it != sess.constEnd(); ++it) {
            KDisplayManager::sess2Str2(*it, user, loc);
            m_model << UserSessionItem(user, loc, (*it).vt, (*it).vt);
        }
    }
    endResetModel();
}

SessionSwitching::SessionSwitching(QObject *parent)
    : QObject (parent)
    , m_sessionModel(new UserSessionsModel(this))
{
}

SessionSwitching::~SessionSwitching()
{
}

bool SessionSwitching::isSwitchUserSupported() const
{
    return KDisplayManager().isSwitchable() && KAuthorized::authorizeKAction(QLatin1String("switch_user"));
}

bool SessionSwitching::isStartNewSessionSupported() const
{
    KDisplayManager dm;
    return dm.isSwitchable() && dm.numReserve() > 0 && KAuthorized::authorizeKAction(QLatin1String("start_new_session"));
}

void SessionSwitching::startNewSession()
{
    // verify that starting a new session is allowed
    if (!isStartNewSessionSupported()) {
        return;
    }

    KDisplayManager().startReserve();
}

void SessionSwitching::activateSession(int index)
{
    // verify that starting a new session is allowed
    if (!isSwitchUserSupported()) {
        return;
    }
    QModelIndex modelIndex(m_sessionModel->index(index));
    if (!modelIndex.isValid()) {
        return;
    }
    KDisplayManager().switchVT(m_sessionModel->data(modelIndex, Qt::UserRole + 2).toInt());
}

}

#include "sessions.moc"
