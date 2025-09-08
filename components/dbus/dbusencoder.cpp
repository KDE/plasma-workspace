/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <dbus/dbus.h>

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusSignature>
#include <algorithm>

#include "dbustype.h"

namespace
{
template<typename T>
QVariant variantListToTypedList(const QVariantList &argList)
{
    QList<T> list;
    std::transform(argList.cbegin(), argList.cend(), std::back_inserter(list), [](const QVariant &variant) {
        return variant.value<T>();
    });
    return QVariant::fromValue(list);
}
}

namespace Encoder
{
QVariant encode(const QVariant &arg, const char *signature)
{
    DBusError error;
    dbus_error_init(&error);
    std::unique_ptr<DBusError, decltype(dbus_error_free) *> guard{&error, dbus_error_free};
    if (!dbus_signature_validate(signature, &error)) {
        return {};
    }

    DBusSignatureIter siter;
    dbus_signature_iter_init(&siter, signature);

    auto encodeMap = [](const QVariant &mapArg, DBusSignatureIter &arraySiter) -> QVariantMap {
        QVariantMap map = mapArg.toMap();
        DBusSignatureIter dictSubSiter;
        dbus_signature_iter_recurse(&arraySiter, &dictSubSiter);
        dbus_signature_iter_next(&dictSubSiter);
        std::unique_ptr<char, decltype(dbus_free) *> dictSig{dbus_signature_iter_get_signature(&dictSubSiter), dbus_free};
        for (auto it = map.begin(); it != map.end(); it = std::next(it)) {
            *it = Encoder::encode(*it, dictSig.get());
        }
        return map;
    };

    std::function<QVariant(const QVariant &, DBusSignatureIter &)> encodeList = [&encodeMap, &encodeList](const QVariant &listArg,
                                                                                                          DBusSignatureIter &arraySiter) -> QVariant {
        QVariantList argList = listArg.toList();
        switch (dbus_signature_iter_get_current_type(&arraySiter)) {
        case DBUS_TYPE_BOOLEAN:
            return variantListToTypedList<bool>(argList);
        case DBUS_TYPE_INT16:
            return variantListToTypedList<short>(argList);
        case DBUS_TYPE_INT32:
            return variantListToTypedList<int>(argList);
        case DBUS_TYPE_INT64:
            return variantListToTypedList<qlonglong>(argList);
        case DBUS_TYPE_UINT16:
            return variantListToTypedList<ushort>(argList);
        case DBUS_TYPE_UINT32:
            return variantListToTypedList<uint>(argList);
        case DBUS_TYPE_UINT64:
            return variantListToTypedList<qulonglong>(argList);
        case DBUS_TYPE_BYTE: {
            QByteArray list;
            list.reserve(argList.size());
            for (const QVariant &arg : std::as_const(argList)) {
                list.append(arg.value<uchar>());
            }
            return list;
        }
        case DBUS_TYPE_STRING:
            return listArg.toStringList();
        case DBUS_TYPE_OBJECT_PATH: {
            QList<QDBusObjectPath> list;
            std::ranges::transform(argList, std::back_inserter(list), [](const QVariant &variant) {
                if (variant.canConvert<QDBusObjectPath>()) {
                    return variant.value<QDBusObjectPath>();
                }
                return QDBusObjectPath(variant.toString());
            });
            return QVariant::fromValue(list);
        }
        case DBUS_TYPE_SIGNATURE: {
            QList<QDBusSignature> list;
            std::ranges::transform(argList, std::back_inserter(list), [](const QVariant &variant) {
                if (variant.canConvert<QDBusSignature>()) {
                    return variant.value<QDBusSignature>();
                }
                return QDBusSignature(variant.toString());
            });
            return QVariant::fromValue(list);
        }
        case DBUS_TYPE_DOUBLE:
            return variantListToTypedList<double>(argList);

        case DBUS_TYPE_ARRAY: {
            // Handle map in array
            DBusSignatureIter arraySiter2;
            // Getting signature of array object
            dbus_signature_iter_recurse(&arraySiter, &arraySiter2);
            if (dbus_signature_iter_get_element_type(&arraySiter) == DBUS_TYPE_DICT_ENTRY) {
                if (argList.empty() || !argList[0].canConvert<QVariantMap>()) {
                    return QVariant::fromValue(QList<QVariantMap>());
                }
                return QVariant::fromValue(QList<QVariantMap>{encodeMap(argList[0], arraySiter2)});
            } else {
                if (argList.empty()) {
                    return encodeList(QVariantList(), arraySiter2);
                }
                return encodeList(argList[0], arraySiter2);
            }
        }
        } // switch

        std::unique_ptr<char, decltype(dbus_free) *> arraySig{dbus_signature_iter_get_signature(&arraySiter), dbus_free};
        for (QVariant &value : argList) {
            value = Encoder::encode(value, arraySig.get());
        }
        return argList;
    };

    switch (dbus_signature_iter_get_current_type(&siter)) {
    case DBUS_TYPE_BOOLEAN:
        return arg.toBool();
    case DBUS_TYPE_INT16:
        return QVariant::fromValue(arg.value<short>()); // NOTE: must use fromValue otherwise the value is treated as normal int
    case DBUS_TYPE_INT32:
        return arg.toInt();
    case DBUS_TYPE_INT64:
        return arg.value<qlonglong>();
    case DBUS_TYPE_UINT16:
        return QVariant::fromValue(arg.value<ushort>());
    case DBUS_TYPE_UINT32:
        return QVariant::fromValue(arg.toUInt());
    case DBUS_TYPE_UINT64:
        return arg.value<qulonglong>();
    case DBUS_TYPE_BYTE:
        return QVariant::fromValue(arg.value<uchar>());
    case DBUS_TYPE_STRING:
        return arg.toString();
    case DBUS_TYPE_OBJECT_PATH: {
        if (arg.canConvert<QDBusObjectPath>()) {
            return arg.value<QDBusObjectPath>();
        }
        return QDBusObjectPath(arg.toString());
    }
    case DBUS_TYPE_SIGNATURE: {
        if (arg.canConvert<QDBusSignature>()) {
            return QVariant::fromValue(arg.value<QDBusSignature>());
        }
        return QVariant::fromValue(QDBusSignature(arg.toString()));
    }
    case DBUS_TYPE_DOUBLE:
        return arg.toDouble();
    case DBUS_TYPE_ARRAY: {
        DBusSignatureIter arraySiter;
        // Getting signature of array object
        dbus_signature_iter_recurse(&siter, &arraySiter);

        if (dbus_signature_iter_get_element_type(&siter) == DBUS_TYPE_DICT_ENTRY) {
            // a{sv} type
            return encodeMap(arg, arraySiter);
        } else {
            // Is a list
            return encodeList(arg, arraySiter);
        }
    }

    case DBUS_TYPE_VARIANT: {
        if (arg.metaType() == QMetaType::fromType<Plasma::DBus::BOOL>()) {
            return QVariant::fromValue(get<Plasma::DBus::BOOL>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::INT16>()) {
            return QVariant::fromValue(get<Plasma::DBus::INT16>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::INT32>()) {
            return QVariant::fromValue(get<Plasma::DBus::INT32>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::INT64>()) {
            return QVariant::fromValue(get<Plasma::DBus::INT64>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::UINT16>()) {
            return QVariant::fromValue(get<Plasma::DBus::UINT16>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::UINT32>()) {
            return QVariant::fromValue(get<Plasma::DBus::UINT32>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::UINT64>()) {
            return QVariant::fromValue(get<Plasma::DBus::UINT64>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::DOUBLE>()) {
            return QVariant::fromValue(get<Plasma::DBus::DOUBLE>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::BYTE>()) {
            return QVariant::fromValue(get<Plasma::DBus::BYTE>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::STRING>()) {
            return QVariant::fromValue(get<Plasma::DBus::STRING>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::SIGNATURE>()) {
            return QVariant::fromValue(get<Plasma::DBus::SIGNATURE>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::OBJECTPATH>()) {
            return QVariant::fromValue(get<Plasma::DBus::OBJECTPATH>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::DICT>()) {
            return QVariant::fromValue(get<Plasma::DBus::DICT>(arg).value);
        } else if (arg.metaType() == QMetaType::fromType<Plasma::DBus::VARIANT>()) {
            return QVariant::fromValue(get<Plasma::DBus::VARIANT>(arg).value);
        }
        return arg;
    }

    case DBUS_TYPE_STRUCT: {
        DBusSignatureIter structSiter;
        dbus_signature_iter_recurse(&siter, &structSiter);
        QVariantList valueList = arg.toList();
        for (QVariant &variant : valueList) {
            std::unique_ptr<char, decltype(dbus_free) *> sig{dbus_signature_iter_get_signature(&structSiter), dbus_free};
            variant = encode(variant, sig.get());
            if (!dbus_signature_iter_next(&structSiter)) {
                break;
            }
        }
        return valueList;
    }

    default:
        [[unlikely]]
        {
            Q_ASSERT_X(false, Q_FUNC_INFO, signature);
            return {};
        }
    } // switch
}
}
