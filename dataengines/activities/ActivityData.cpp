/*
    SPDX-FileCopyrightText: 2011 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ActivityData.h"

#include <QDBusMetaType>
#include <QMetaType>

class ActivityDataStaticInit
{
public:
    ActivityDataStaticInit()
    {
        qDBusRegisterMetaType<ActivityData>();
        qDBusRegisterMetaType<QList<ActivityData>>();
    }

    static ActivityDataStaticInit _instance;
};

ActivityDataStaticInit ActivityDataStaticInit::_instance;

ActivityData::ActivityData()
{
}

ActivityData::ActivityData(const ActivityData &source)
    : id(source.id)
{
    score = source.score;
}

ActivityData &ActivityData::operator=(const ActivityData &source)
{
    if (&source != this) {
        score = source.score;
        id = source.id;
    }

    return *this;
}

QDBusArgument &operator<<(QDBusArgument &arg, const ActivityData r)
{
    arg.beginStructure();

    arg << r.id;
    arg << r.score;

    arg.endStructure();

    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, ActivityData &r)
{
    arg.beginStructure();

    arg >> r.id;
    arg >> r.score;

    arg.endStructure();

    return arg;
}

QDebug operator<<(QDebug dbg, const ActivityData &r)
{
    dbg << "ActivityData(" << r.score << r.id << ")";
    return dbg.space();
}
