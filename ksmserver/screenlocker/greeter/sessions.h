/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

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
#ifndef SCREENLOCKER_SESSIONS_H
#define SCREENLOCKER_SESSIONS_H

#include <QtCore/QAbstractItemModel>

namespace ScreenLocker
{

class UserSessionsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    UserSessionsModel(QObject *parent = 0);
    virtual ~UserSessionsModel();

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

private:
    class UserSessionItem;
    void init();
    QList<UserSessionItem> m_model;
};

class UserSessionsModel::UserSessionItem
{
public:
    UserSessionItem(const QString &session, const QString &location, int vt, bool enabled)
        : m_session(session)
        , m_location(location)
        , m_vt(vt)
        , m_enabled(enabled)
    {}
    QString m_session;
    QString m_location;
    int m_vt;
    bool m_enabled;
};

class SessionSwitching : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool switchUserSupported READ isSwitchUserSupported CONSTANT)
    Q_PROPERTY(bool startNewSessionSupported READ isStartNewSessionSupported CONSTANT)
    Q_PROPERTY(QAbstractItemModel* model READ sessionModel)
public:
    SessionSwitching(QObject *parent = NULL);
    virtual ~SessionSwitching();

    QAbstractItemModel *sessionModel() {
        return m_sessionModel;
    }

    /**
     * @returns @c true if switching between user sessions is possible, @c false otherwise.
     **/
    bool isSwitchUserSupported() const;
    /**
     * @returns @c true if a new session can be started, @c false otherwise.
     **/
    bool isStartNewSessionSupported() const;
public Q_SLOTS:
    /**
     * Invoke to start a new session if allowed.
     **/
    void startNewSession();
    void activateSession(int index);
private:
    UserSessionsModel *m_sessionModel;
};

} // namespace
Q_DECLARE_METATYPE(ScreenLocker::UserSessionsModel*)
#endif // SCREENLOCKER_SESSIONS_H
