/*
    SPDX-FileCopyrightText: 2011 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDBusArgument>
#include <QDebug>
#include <QObject>
#include <QString>

class ActivityData
{
public:
    ActivityData();
    ActivityData(const ActivityData &source);
    ActivityData &operator=(const ActivityData &source);

    double score;
    QString id;
};

typedef QList<ActivityData> ActivityDataList;
Q_DECLARE_METATYPE(ActivityData)
Q_DECLARE_METATYPE(ActivityDataList)

QDBusArgument &operator<<(QDBusArgument &arg, const ActivityData);
const QDBusArgument &operator>>(const QDBusArgument &arg, ActivityData &rec);

QDebug operator<<(QDebug dbg, const ActivityData &r);
