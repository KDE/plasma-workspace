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

#include "ActivityData.h"

#include <QMetaType>
#include <QDBusMetaType>

class ActivityDataStaticInit {
public:
    ActivityDataStaticInit()
    {
        qDBusRegisterMetaType < ActivityData > ();
        qDBusRegisterMetaType < QList < ActivityData > > ();
    }

    static ActivityDataStaticInit _instance;

};

ActivityDataStaticInit ActivityDataStaticInit::_instance;

ActivityData::ActivityData()
{
}

ActivityData::ActivityData(const ActivityData & source)
{
    score       = source.score;
    id          = source.id;
}

ActivityData & ActivityData::operator = (const ActivityData & source)
{
    if (&source != this) {
        score       = source.score;
        id          = source.id;
    }

    return *this;
}

QDBusArgument & operator << (QDBusArgument & arg, const ActivityData r)
{
    arg.beginStructure();

    arg << r.id;
    arg << r.score;

    arg.endStructure();

    return arg;
}

const QDBusArgument & operator >> (const QDBusArgument & arg, ActivityData & r)
{
    arg.beginStructure();

    arg >> r.id;
    arg >> r.score;

    arg.endStructure();

    return arg;
}

QDebug operator << (QDebug dbg, const ActivityData & r)
{
    dbg << "ActivityData(" << r.score << r.id << ")";
    return dbg.space();
}
