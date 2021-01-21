/*
 * SNI Dbus serialisers
 * Copyright 2015  <davidedmundson@kde.org> David Edmundson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SNIDBUS_H
#define SNIDBUS_H

#include <QByteArray>
#include <QDBusArgument>
#include <QImage>
#include <QString>
#include <QVector>

// Custom message type for DBus
struct KDbusImageStruct {
    KDbusImageStruct();
    KDbusImageStruct(const QImage &image);
    int width;
    int height;
    QByteArray data;
};

typedef QVector<KDbusImageStruct> KDbusImageVector;

struct KDbusToolTipStruct {
    QString icon;
    KDbusImageVector image;
    QString title;
    QString subTitle;
};

const QDBusArgument &operator<<(QDBusArgument &argument, const KDbusImageStruct &icon);
const QDBusArgument &operator>>(const QDBusArgument &argument, KDbusImageStruct &icon);

Q_DECLARE_METATYPE(KDbusImageStruct)

const QDBusArgument &operator<<(QDBusArgument &argument, const KDbusImageVector &iconVector);
const QDBusArgument &operator>>(const QDBusArgument &argument, KDbusImageVector &iconVector);

Q_DECLARE_METATYPE(KDbusImageVector)

const QDBusArgument &operator<<(QDBusArgument &argument, const KDbusToolTipStruct &toolTip);
const QDBusArgument &operator>>(const QDBusArgument &argument, KDbusToolTipStruct &toolTip);

Q_DECLARE_METATYPE(KDbusToolTipStruct)

#endif // SNIDBUS_H
