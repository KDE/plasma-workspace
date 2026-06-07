// SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimeZone>

#include "timezoneutils.h"

QList<QByteArray> TimeZoneUtils::availableTimeZoneIds() const
{
    return QTimeZone::availableTimeZoneIds();
}

QVariantMap TimeZoneUtils::bandData() const
{
    const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("timezonefiles/timezones.json"));
    if (path.isEmpty()) {
        return {};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = doc.object();
    const QJsonArray features = root.value(QStringLiteral("features")).toArray();
    const QJsonObject bandGroups = root.value(QStringLiteral("bandGroups")).toObject();

    QVariantMap tzids;
    QVariantMap groups;

    for (const QJsonValue &featVal : features) {
        const QJsonObject props = featVal.toObject().value(QStringLiteral("properties")).toObject();
        const QString tzid = props.value(QStringLiteral("tzid")).toString();
        if (!tzid.isEmpty()) {
            tzids.insert(tzid, props.toVariantMap());
        }
    }

    for (auto it = bandGroups.begin(); it != bandGroups.end(); ++it) {
        groups.insert(it.key(), it.value().toObject().toVariantMap());
    }

    QVariantMap result;
    result.insert(QStringLiteral("tzids"), tzids);
    result.insert(QStringLiteral("groups"), groups);
    return result;
}
