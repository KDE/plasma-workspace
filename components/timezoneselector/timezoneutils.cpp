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
    const QJsonObject groupOutlines = root.value(QStringLiteral("groupOutlines")).toObject();

    QVariantMap tzids;
    QVariantMap outlines;

    for (const QJsonValue &featVal : features) {
        const QJsonObject props = featVal.toObject().value(QStringLiteral("properties")).toObject();
        const QString tzid = props.value(QStringLiteral("tzid")).toString();
        if (!tzid.isEmpty()) {
            tzids.insert(tzid, props.toVariantMap());
        }
    }

    for (auto it = groupOutlines.begin(); it != groupOutlines.end(); ++it) {
        QVariantList rings;
        const QJsonArray ringsArray = it.value().toArray();
        for (const QJsonValue &ringVal : ringsArray) {
            QVariantList ring;
            const QJsonArray ringArray = ringVal.toArray();
            for (const QJsonValue &ptVal : ringArray) {
                const QJsonArray pt = ptVal.toArray();
                if (pt.size() >= 2) {
                    QVariantList coord;
                    coord.append(pt[0].toDouble());
                    coord.append(pt[1].toDouble());
                    ring.append(QVariant(coord));
                }
            }
            rings.append(QVariant(ring));
        }
        outlines.insert(it.key(), rings);
    }

    QVariantMap result;
    result.insert(QStringLiteral("tzids"), tzids);
    result.insert(QStringLiteral("outlines"), outlines);
    return result;
}
