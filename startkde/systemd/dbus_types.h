// SPDX-FileCopyrightText: 2013 Andrea Scarpino <me@andreascarpino.it>
// SPDX-FileCopyrightText: 2020 Henri Chain <henri.chain@enioka.com>
// SPDX-FileCopyrightText: 2020 Kevin Ottens <kevin.ottens@enioka.com>

#pragma once

#include <QString>

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusMetaType>

using ManagerDBusUnit = struct ManagerDBusUnit {
    QString id;
    QString description;
    QString loadState;
    QString activeState;
    QString subState;
    QString following;
    QDBusObjectPath path;
    quint32 jobId;
    QString jobType;
    QDBusObjectPath jobPath;
};
Q_DECLARE_METATYPE(ManagerDBusUnit)
using ManagerDBusUnitList = QList<ManagerDBusUnit>;
Q_DECLARE_METATYPE(ManagerDBusUnitList)

inline QDBusArgument &operator<<(QDBusArgument &argument, const ManagerDBusUnit &unit)
{
    argument.beginStructure();
    argument << unit.id << unit.description << unit.loadState << unit.activeState;
    argument << unit.subState << unit.following << unit.path << unit.jobId;
    argument << unit.jobType << unit.jobPath;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, ManagerDBusUnit &unit)
{
    argument.beginStructure();
    argument >> unit.id >> unit.description >> unit.loadState >> unit.activeState;
    argument >> unit.subState >> unit.following >> unit.path >> unit.jobId;
    argument >> unit.jobType >> unit.jobPath;
    argument.endStructure();
    return argument;
}
