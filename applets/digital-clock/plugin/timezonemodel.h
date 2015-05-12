/***************************************************************************
 *   Copyright (C) 2014 Kai Uwe Broulik <kde@privat.broulik.de>            *
 *   Copyright (C) 2014  Martin Klapetek <mklapetek@kde.org>               *
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

#ifndef TIMEZONEMODEL_H
#define TIMEZONEMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "timezonedata.h"

class TimezonesI18n;

class TimeZoneFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterString WRITE setFilterString MEMBER m_filterString NOTIFY filterStringChanged)

public:
    explicit TimeZoneFilterProxy(QObject *parent = 0);
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

    void setFilterString(const QString &filterString);

Q_SIGNALS:
    void filterStringChanged();

private:
    QString m_filterString;
    QStringMatcher m_stringMatcher;
};

//=============================================================================

class TimeZoneModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList selectedTimeZones WRITE setSelectedTimeZones MEMBER m_selectedTimeZones NOTIFY selectedTimeZonesChanged)

public:
    explicit TimeZoneModel(QObject *parent = 0);
    ~TimeZoneModel();

    enum Roles {
        TimeZoneIdRole = Qt::UserRole + 1,
        RegionRole,
        CityRole,
        CommentRole,
        CheckedRole
    };

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    void update();
    void setSelectedTimeZones(const QStringList &selectedTimeZones);

    Q_INVOKABLE void selectLocalTimeZone();

Q_SIGNALS:
    void selectedTimeZonesChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    QList<TimeZoneData> m_data;
    QStringList m_selectedTimeZones;
    TimezonesI18n *m_timezonesI18n;
};

#endif // TIMEZONEMODEL_H
