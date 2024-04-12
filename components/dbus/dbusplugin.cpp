/*
     SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

     SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QDBusMetaType>
#include <QQmlExtensionPlugin>

#include "dbustype.h"

void qml_register_types_org_kde_plasma_workspace_dbus();

class DBusPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
    Q_DISABLE_COPY_MOVE(DBusPlugin)
public:
    DBusPlugin(QObject *parent = nullptr)
        : QQmlEngineExtensionPlugin(parent)
    {
        QMetaType::registerConverter<Plasma::DBus::BOOL, bool>();
        QMetaType::registerConverter<Plasma::DBus::INT16, short>();
        QMetaType::registerConverter<Plasma::DBus::INT32, int>();
        QMetaType::registerConverter<Plasma::DBus::INT64, qlonglong>();
        QMetaType::registerConverter<Plasma::DBus::UINT16, ushort>();
        QMetaType::registerConverter<Plasma::DBus::UINT32, uint>();
        QMetaType::registerConverter<Plasma::DBus::UINT64, qulonglong>();
        QMetaType::registerConverter<Plasma::DBus::DOUBLE, double>();
        QMetaType::registerConverter<Plasma::DBus::BYTE, uchar>();
        QMetaType::registerConverter<Plasma::DBus::STRING, QString>();
        QMetaType::registerConverter<Plasma::DBus::SIGNATURE, QDBusSignature>();
        QMetaType::registerConverter<QDBusSignature, QString>([](const QDBusSignature &signature) -> QString {
            return signature.signature();
        });
        QMetaType::registerConverter<Plasma::DBus::OBJECTPATH, QDBusObjectPath>();
        QMetaType::registerConverter<QDBusObjectPath, QString>([](const QDBusObjectPath &objectPath) -> QString {
            return objectPath.path();
        });
        QMetaType::registerConverter<Plasma::DBus::DICT, QVariantMap>();
        QMetaType::registerConverter<Plasma::DBus::VARIANT, QDBusVariant>();
        QMetaType::registerConverter<Plasma::DBus::VARIANT, QVariant>([](const Plasma::DBus::VARIANT &variant) -> QVariant {
            return variant.value.variant();
        });

        qDBusRegisterMetaType<QList<QMap<QString, QVariant>>>();

        volatile auto registration = &qml_register_types_org_kde_plasma_workspace_dbus;
        Q_UNUSED(registration);
    }
};

#include "dbusplugin.moc"
