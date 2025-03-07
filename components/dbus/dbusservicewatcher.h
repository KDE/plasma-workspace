/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QBindable>
#include <QDBusServiceWatcher>
#include <QQmlParserStatus>
#include <qqmlregistration.h>

namespace BusType
{
QML_ELEMENT
Q_NAMESPACE //
    enum Type {
        Session = 0,
        System,
    };
Q_ENUM_NS(Type)
}

/*!
    \qmltype DBusServiceWatcher
    \inherits QtObject
    \inqmlmodule org.kde.plasma.workspace.dbus
    \brief The DBusServiceWatcher class watches for the availability of D-Bus services.

    The DBusServiceWatcher class allows you to monitor the availability of specific D-Bus services.
    You can specify which services to watch and react to events such as service registration and unregistration.

    The following example shows how you might monitor a D-Bus service in QML.

    \qml
    DBusServiceWatcher {
        id: effectWatcher
        busType: BusType.Session
        watchedService: "org.kde.KWin.Effect.WindowView1"
    }

    property bool hasWindowViewEffect: effectWatcher.registered // Whether the KWin effect is enabled
    \endqml
*/
class DBusServiceWatcher : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    /*!
        \qmlproperty BusType::Type DBusServiceWatcher::busType

        The type of the bus connection. The valid bus types are \c Session and \c System
     */
    Q_PROPERTY(BusType::Type busType READ busType WRITE setBusType NOTIFY busTypeChanged)

    /*!
        \qmlproperty string DBusServiceWatcher::watchedService

        The D-Bus service name that the watcher is currently monitoring.
     */
    Q_PROPERTY(QString watchedService READ watchedService WRITE setWatchedService NOTIFY watchedServiceChanged)

    /*!
        \qmlproperty bool DBusServiceWatcher::registered

        Whether the watched service is registered on D-Bus.
     */
    Q_PROPERTY(bool registered READ default NOTIFY registeredChanged BINDABLE isRegistered)

public:
    explicit DBusServiceWatcher(QObject *parent = nullptr);
    ~DBusServiceWatcher() override;

    BusType::Type busType() const;
    void setBusType(BusType::Type type);

    QBindable<bool> isRegistered() const;

    QString watchedService() const;
    void setWatchedService(const QString &service);

Q_SIGNALS:
    void busTypeChanged();
    void watchedServiceChanged();
    void registeredChanged();

private:
    void classBegin() override;
    void componentComplete() override;

    void onServiceRegistered(const QString &serviceName);
    void onServiceUnregistered(const QString &serviceName);
    void checkServiceRegistered();

    bool m_ready = false;
    QDBusServiceWatcher m_watcher;
    QPropertyNotifier m_watchedServiceNotifier;
    BusType::Type m_busType = BusType::Session;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DBusServiceWatcher, bool, m_registered, false, &DBusServiceWatcher::registeredChanged)
};
