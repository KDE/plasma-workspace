/*
    SPDX-FileCopyrightText: 2012 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusdecoder.h"

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusSignature>
#include <QDBusVariant>

#include "dbustype.h"

namespace Decoder
{
QVariant dbusToVariant(const QVariant &variant)
{
    if (variant.metaType() == QMetaType::fromType<QDBusArgument>()) {
        const auto argument = variant.value<QDBusArgument>();
        switch (argument.currentType()) {
        case QDBusArgument::BasicType:
        case QDBusArgument::MapEntryType:
            return dbusToVariant(argument.asVariant());
        case QDBusArgument::VariantType:
            return dbusToVariant(argument.asVariant().value<QDBusVariant>().variant());
        case QDBusArgument::ArrayType: {
            QVariantList array;
            argument.beginArray();
            while (!argument.atEnd()) {
                const QVariant value = argument.asVariant();
                array.append(dbusToVariant(value));
            }
            argument.endArray();
            return array;
        }
        case QDBusArgument::StructureType: {
            QVariantList structure;
            argument.beginStructure();
            while (!argument.atEnd()) {
                const QVariant value = argument.asVariant();
                structure.append(dbusToVariant(value));
            }
            argument.endStructure();
            return structure;
        }
        case QDBusArgument::MapType: {
            QVariantMap map;
            argument.beginMap();
            while (!argument.atEnd()) {
                argument.beginMapEntry();
                const QVariant key = argument.asVariant();
                const QVariant value = argument.asVariant();
                argument.endMapEntry();
                map.insert(key.toString(), dbusToVariant(value));
            }
            argument.endMap();
            return map;
        }
        default:
            return variant;
        }
    } else if (variant.metaType() == QMetaType::fromType<QDBusObjectPath>()) {
        return Plasma::DBus::OBJECTPATH(get<QDBusObjectPath>(variant).path());
    } else if (variant.metaType() == QMetaType::fromType<QDBusSignature>()) {
        return Plasma::DBus::SIGNATURE(get<QDBusSignature>(variant).signature());
    } else if (variant.metaType() == QMetaType::fromType<QDBusVariant>()) {
        return dbusToVariant(get<QDBusVariant>(variant).variant());
    } else {
        switch (variant.typeId()) {
        case QMetaType::Bool:
            return variant; // QML always sees object as true
        case QMetaType::Short:
            return Plasma::DBus::INT16(get<short>(variant));
        case QMetaType::Int:
            return Plasma::DBus::INT32(get<int>(variant));
        case QMetaType::LongLong:
            return Plasma::DBus::INT64(get<qlonglong>(variant));
        case QMetaType::UShort:
            return Plasma::DBus::UINT16(get<ushort>(variant));
        case QMetaType::UInt:
            return Plasma::DBus::UINT32(get<uint>(variant));
        case QMetaType::ULongLong:
            return Plasma::DBus::UINT64(get<qulonglong>(variant));
        case QMetaType::Double:
            return Plasma::DBus::DOUBLE(get<double>(variant));
        case QMetaType::UChar:
            return Plasma::DBus::BYTE(get<uchar>(variant));
        case QMetaType::QString:
            return Plasma::DBus::STRING(get<QString>(variant));
        case QMetaType::QByteArray: {
            const auto &bytes = get<QByteArray>(variant);
            return QVariant::fromValue(QList<Plasma::DBus::BYTE>{bytes.cbegin(), bytes.cend()});
        }
        }
    }

    return variant;
}

QVariantList decode(const QDBusMessage &message)
{
    QVariantList arguments = message.arguments();
    for (QVariant &arg : arguments) {
        arg = dbusToVariant(arg);
    }
    return arguments;
}
}
