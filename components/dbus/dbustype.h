/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusObjectPath>
#include <QDBusSignature>
#include <QDBusVariant>
#include <QJSValue>
#include <QObject>
#include <QVariantMap>
#include <qqmlregistration.h>

#define DBUS_QML_TYPE(CLASSNAME, TYPENAME, CONSTRUCTTYPE, STORETYPE, DBUSSIGNATURE)                                                                            \
    class CLASSNAME                                                                                                                                            \
    {                                                                                                                                                          \
        Q_GADGET                                                                                                                                               \
        QML_VALUE_TYPE(TYPENAME)                                                                                                                               \
        QML_CONSTRUCTIBLE_VALUE                                                                                                                                \
        Q_PROPERTY(STORETYPE value MEMBER value)                                                                                                               \
    public:                                                                                                                                                    \
        explicit CLASSNAME()                                                                                                                                   \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        Q_INVOKABLE explicit CLASSNAME(CONSTRUCTTYPE value)                                                                                                    \
            : value(value)                                                                                                                                     \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        operator STORETYPE() const                                                                                                                             \
        {                                                                                                                                                      \
            return value;                                                                                                                                      \
        }                                                                                                                                                      \
        Q_INVOKABLE QString toString() const                                                                                                                   \
        {                                                                                                                                                      \
            QString result;                                                                                                                                    \
            QMetaType::convert(QMetaType::fromType<STORETYPE>(), &value, QMetaType(QMetaType::QString), &result);                                              \
            return result;                                                                                                                                     \
        }                                                                                                                                                      \
        operator QVariant() const                                                                                                                              \
        {                                                                                                                                                      \
            return QVariant::fromValue(*this);                                                                                                                 \
        }                                                                                                                                                      \
        STORETYPE value;                                                                                                                                       \
        constexpr static const char *signature = DBUSSIGNATURE;                                                                                                \
    };

namespace Plasma::DBus
{
DBUS_QML_TYPE(BOOL, bool, bool, bool, "b")
DBUS_QML_TYPE(INT16, int16, int, short, "n")
DBUS_QML_TYPE(INT32, int32, int, int, "i")
DBUS_QML_TYPE(INT64, int64, double, qlonglong, "x")
DBUS_QML_TYPE(UINT16, uint16, int, ushort, "q")
DBUS_QML_TYPE(UINT32, uint32, double, uint, "u")
DBUS_QML_TYPE(UINT64, uint64, double, qulonglong, "t")
DBUS_QML_TYPE(DOUBLE, double, double, double, "d")
DBUS_QML_TYPE(BYTE, byte, int, uchar, "y")
DBUS_QML_TYPE(STRING, string, QString, QString, "s")
DBUS_QML_TYPE(SIGNATURE, signature, QString, QDBusSignature, "g")
DBUS_QML_TYPE(OBJECTPATH, objectPath, QString, QDBusObjectPath, "o")
DBUS_QML_TYPE(DICT, dict, QVariantMap, QVariantMap, "a{sv}")

class VARIANT
{
    Q_GADGET
    QML_VALUE_TYPE(variant)
    QML_CONSTRUCTIBLE_VALUE
    Q_PROPERTY(QDBusVariant value MEMBER value)
public:
    explicit VARIANT()
    {
    }
#if (QT_VERSION == QT_VERSION_CHECK(6, 8, 0)) // https://bugreports.qt.io/browse/QTBUG-130522
    Q_INVOKABLE explicit VARIANT(const QVariant &_value)
        : value(_value)
#else
    Q_INVOKABLE explicit VARIANT(const QJSValue &value)
        : value(value.toVariant())
#endif
    {
    }
    operator QVariant() const
    {
        return value.variant();
    }
    operator QDBusVariant() const
    {
        return value;
    }
    Q_INVOKABLE QString toString() const
    {
        return value.variant().toString();
    }
    QDBusVariant value;
    constexpr static const char *signature = "v";
};
}
