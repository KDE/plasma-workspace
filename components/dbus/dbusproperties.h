/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QQmlPropertyMap>
#include <qqmlregistration.h>

#include "dbusservicewatcher.h"

class QDBusPendingCallWatcher;

namespace Plasma
{
/*!
    \class Properties
    \brief The Properties class allows you to read D-Bus interface properties in QML.
    \inmodule org.kde.plasma.workspace.dbus

    The following example shows how you might access a property in QML.

    \code
    Properties {
        id: plasmashellProps
        busType: DBus.BusType.Session
        service: "org.kde.plasmashell"
        path: "/PlasmaShell"
        interface: "org.kde.PlasmaShell"
    }

    property int accentColor: plasmashellProps.properties.color
    property bool editMode: plasmashellProps.properties.editMode
    \endcode
*/
class DBusProperties : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(Properties)
    Q_DISABLE_COPY_MOVE(DBusProperties)

    /**
     * The type of the bus connection. The valid bus types are @c Session and @c System
     */
    Q_PROPERTY(BusType::Type busType READ busType WRITE setBusType NOTIFY busTypeChanged)

    /**
     * The bus address of the method call
     */
    Q_PROPERTY(QString service READ service WRITE setService NOTIFY serviceChanged)

    /**
     * The object path of the method call is being sent to
     */
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)

    /**
     * The interface of the method call
     */
    Q_PROPERTY(QString iface READ interface WRITE setInterface NOTIFY interfaceChanged)

    /**
     * The properties of the specific interface
     */
    Q_PROPERTY(QQmlPropertyMap *properties READ properties NOTIFY propertyMapChanged)

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

    QQmlPropertyMap *properties() const;
    void resetProperties();

    /**
     * Manually update a property whose name is @p key
     */
    Q_INVOKABLE void update(const QString &key);

    /**
     * Manually update all properties
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

private Q_SLOTS:
    void updateProperties(const QString &interfaceName = {}, const QVariantMap &changedProperties = {}, const QStringList &invalidatedProperties = {});

private:
    void classBegin() override;
    void componentComplete() override;

    bool isValid() const;
    void refreshCallback(QDBusPendingCallWatcher *watcher);
    void connectToPropertiesChangedSignal();
    void disconnectFromPropertiesChangedSignal();

    bool m_ready = false;

    BusType::Type m_busType = BusType::Session;
    QString m_service;
    QString m_path;
    QString m_interface;
    QQmlPropertyMap *m_properties;

    QPointer<QDBusPendingCallWatcher> m_refreshWatcher;
    std::unordered_map<QString, QDBusPendingCallWatcher *> m_updateKeyWatchers;
};
}
