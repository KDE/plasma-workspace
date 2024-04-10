/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <dbus/dbus.h>

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusSignature>

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

    QVariant result;
    DBusSignatureIter siter;
    dbus_signature_iter_init(&siter, signature);
    switch (dbus_signature_iter_get_current_type(&siter)) {
    case DBUS_TYPE_BOOLEAN:
        return arg.toBool();
    case DBUS_TYPE_INT16:
        return static_cast<short>(arg.toInt());
    case DBUS_TYPE_INT32:
        return arg.toInt();
    case DBUS_TYPE_INT64:
        return static_cast<qlonglong>(arg.toInt());
    case DBUS_TYPE_UINT16:
        return static_cast<ushort>(arg.toUInt());
    case DBUS_TYPE_UINT32:
        return arg.toUInt();
    case DBUS_TYPE_UINT64:
        return static_cast<qulonglong>(arg.toUInt());
    case DBUS_TYPE_BYTE:
        return static_cast<uchar>(arg.toUInt());
    case DBUS_TYPE_STRING:
        return arg.toString();
    case DBUS_TYPE_OBJECT_PATH:
        return QDBusObjectPath(arg.toString());
    case DBUS_TYPE_SIGNATURE:
        return QVariant::fromValue(QDBusSignature(arg.toString()));
    case DBUS_TYPE_DOUBLE:
        return arg.toDouble();
    case DBUS_TYPE_ARRAY: {
        if (dbus_signature_iter_get_element_type(&siter) == DBUS_TYPE_DICT_ENTRY && arg.typeId() == QMetaType::QVariantMap) {
            // a{sv} type
            return arg.toMap();
        } else {
            // Is a list
            DBusSignatureIter arraySiter;
            // Getting signature of array object
            dbus_signature_iter_recurse(&siter, &arraySiter);
            QVariantList argList = arg.toList();
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
            case DBUS_TYPE_BYTE:
                return variantListToTypedList<uchar>(argList);
            case DBUS_TYPE_STRING:
                return arg.toStringList();
            case DBUS_TYPE_OBJECT_PATH: {
                QList<QDBusObjectPath> list;
                std::transform(argList.cbegin(), argList.cend(), std::back_inserter(list), [](const QVariant &variant) {
                    return QDBusObjectPath(variant.toString());
                });
                return QVariant::fromValue(list);
            }
            case DBUS_TYPE_SIGNATURE: {
                QList<QDBusSignature> list;
                std::transform(argList.cbegin(), argList.cend(), std::back_inserter(list), [](const QVariant &variant) {
                    return QDBusSignature(variant.toString());
                });
                return QVariant::fromValue(list);
            }
            case DBUS_TYPE_DOUBLE:
                return variantListToTypedList<double>(argList);
            } // switch

            std::unique_ptr<char, decltype(dbus_free) *> arraySig{dbus_signature_iter_get_signature(&arraySiter), dbus_free};
            // For more complex array like "a(uuaa{sv})"
            for (QVariant &value : argList) {
                value = Encoder::encode(value, arraySig.get());
            }
            return argList;
        }
    }

    case DBUS_TYPE_VARIANT:
        return QVariant::fromValue(QDBusVariant(arg));

    case DBUS_TYPE_STRUCT: {
        DBusSignatureIter structSiter;
        dbus_signature_iter_recurse(&siter, &structSiter);
        QVariantList valueList = arg.toList();
        if (valueList.empty()) {
            std::unique_ptr<char, decltype(dbus_free) *> sig{dbus_signature_iter_get_signature(&structSiter), dbus_free};
            return encode(QVariant(), sig.get());
        }
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
        return {};
    } // switch
}
}
