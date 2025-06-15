/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusMessage>
#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QQmlPropertyMap>
#include <qqmlregistration.h>

#include "dbusservicewatcher.h"

class QDBusPendingCallWatcher;

namespace Plasma
{
class DBusProperties;

class DBusPropertyMap : public QQmlPropertyMap
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DBusPropertyMap)
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    DBusPropertyMap(QObject *parent = nullptr);

    void update(const QString &key);
    void updateAll();
    void updateIntrospection();
    void updateProperties(const QString &interfaceName = {}, const QVariantMap &changedProperties = {}, const QStringList &invalidatedProperties = {});

private:
    QVariant updateValue(const QString &key, const QVariant &input) override;

    QByteArray propertySignature(QStringView key);

    void refreshCallback(QDBusPendingCallWatcher *watcher);

    QString m_introspection;

    std::unique_ptr<QDBusPendingCallWatcher> m_refreshWatcher;
    std::unique_ptr<QDBusPendingCallWatcher> m_introspectWatcher;
    std::unordered_map<QString /*Key*/, std::unique_ptr<QDBusPendingCallWatcher>> m_pendingGetProps;
    std::unordered_map<QString /*Key*/, std::unique_ptr<QDBusPendingCallWatcher, QScopedPointerDeleteLater>> m_pendingSetProps;

    DBusProperties *q;
};

/*!
    \class Properties
    \brief The Properties class allows you to read and modify D-Bus interface properties in QML.
    \inmodule org.kde.plasma.workspace.dbus

    The following example shows how you might access a property in QML.

    \qml
    Properties {
        id: plasmashellProps
        busType: DBus.BusType.Session
        service: "org.kde.plasmashell"
        path: "/PlasmaShell"
        interface: "org.kde.PlasmaShell"
    }

    property bool editMode: Boolean(plasmashellProps.properties.editMode)
    \endqml
*/
class DBusProperties : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(Properties)
    Q_DISABLE_COPY_MOVE(DBusProperties)

    /*!
        \qmlproperty BusType::Type Properties::busType

        The type of the bus connection. The valid bus types are \c Session and \c System
     */
    Q_PROPERTY(BusType::Type busType READ busType WRITE setBusType NOTIFY busTypeChanged)

    /*!
        \qmlproperty string Properties::service

        The bus address of the method call
     */
    Q_PROPERTY(QString service READ service WRITE setService NOTIFY serviceChanged)

    /*!
        \qmlproperty string Properties::path

        The object path of the method call is being sent to
     */
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)

    /*!
        \qmlproperty string Properties::iface

        The interface of the method call
     */
    Q_PROPERTY(QString iface READ interface WRITE setInterface NOTIFY interfaceChanged)

    /*!
        \qmlproperty QQmlPropertyMap Properties::properties

        The properties of the specific interface
     */
    Q_PROPERTY(Plasma::DBusPropertyMap *properties READ properties NOTIFY propertyMapChanged)

public:
    explicit DBusProperties(QObject *parent = nullptr);

    BusType::Type busType() const;
    void setBusType(BusType::Type type);

    QString service() const;
    void setService(const QString &value);

    QString path() const;
    void setPath(const QString &path);

    QString interface() const;
    void setInterface(const QString &iface);

    DBusPropertyMap *properties() const;
    void resetProperties();

    /*!
        \qmlmethod void org.kde.plasma.workspace.dbus::Properties::update(key)

        Manually update a property whose name is \a key
     */
    Q_INVOKABLE void update(const QString &key);

    /*!
        \qmlmethod void org.kde.plasma.workspace.dbus::Properties::updateAll()

        Manually update all properties
     */
    Q_INVOKABLE void updateAll();

Q_SIGNALS:
    void busTypeChanged();
    void serviceChanged();
    void pathChanged();
    void interfaceChanged();
    void propertyMapChanged();

    // Proxy for org.freedesktop.DBus.Properties.PropertiesChanged
    void propertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void refreshed();

private Q_SLOTS:
    void updateProperties(const QString &interfaceName = {}, const QVariantMap &changedProperties = {}, const QStringList &invalidatedProperties = {});

private:
    void classBegin() override;
    void componentComplete() override;

    bool isValid() const;
    void connectToPropertiesChangedSignal();
    void disconnectFromPropertiesChangedSignal();

    BusType::Type m_busType = BusType::Session;
    QString m_service;
    QString m_path;
    QString m_interface;

    bool m_ready = false;
    DBusPropertyMap *m_properties;

    friend class DBusPropertyMap;
};
}
