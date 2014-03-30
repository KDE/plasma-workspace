/*
 *   Copyright (C) 2011 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ACTIVITY_DATA_H_
#define ACTIVITY_DATA_H_

#include <QObject>
#include <QString>
#include <QtDBus/QDBusArgument>
#include <QDebug>

class ActivityData {
public:
    ActivityData();
    ActivityData(const ActivityData & source);
    ActivityData & operator = (const ActivityData & source);

    double score;
    QString id;

};

typedef QList<ActivityData> ActivityDataList;
Q_DECLARE_METATYPE(ActivityData)
Q_DECLARE_METATYPE(ActivityDataList)

QDBusArgument & operator << (QDBusArgument & arg, const ActivityData);
const QDBusArgument & operator >> (const QDBusArgument & arg, ActivityData & rec);

QDebug operator << (QDebug dbg, const ActivityData & r);

#endif // ACTIVITY_DATA_H_
