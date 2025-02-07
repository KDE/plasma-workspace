/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDBusMessage>
#include <QObject>
#include <QQmlParserStatus>
#include <qqmlregistration.h>

#include "dbusservicewatcher.h"

class QDBusMessage;

namespace Plasma
{
class DBusSignalWatcher;

/*!
    \qmltype SignalWatcher
    \inherits QtObject
    \inqmlmodule org.kde.plasma.workspace.dbus
    \brief The SignalWatcher class creates a listener to D-Bus signals in QML.

    The following example shows how you might connect to a D-Bus signal named "wallpaperChanged" in QML.
    \note The function name of the signal handler needs to be prefixed with "dbus".

    \qml
    SignalWatcher {
        busType: BusType.Session
        service: "org.kde.plasmashell"
        path: "/PlasmaShell"
        iface: "org.kde.PlasmaShell"
        function dbuswallpaperChanged(screenNum) {
            console.log(screenNum)
        }
    }
   \endqml
*/
class DBusSignalWatcher : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(SignalWatcher)
    Q_DISABLE_COPY_MOVE(DBusSignalWatcher)

    /*!
        \qmlproperty bool SignalWatcher::enabled

        This property holds whether the item accepts change events.
        By default, this property is \c true.
     */
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    /*!
        \qmlproperty var SignalWatcher::busType

        The type of the bus connection. The valid bus types are \c Session , \c System .
        If the input value is a string, the string is regarded as a bus address.
     */
    Q_PROPERTY(QVariant busType READ busType WRITE setBusType NOTIFY busTypeChanged)

    /**
        \qmlproperty string SignalWatcher::service

        The bus address of the method call.
     */
    Q_PROPERTY(QString service READ service WRITE setService NOTIFY serviceChanged)

    /*!
        \qmlproperty string SignalWatcher::path

        The object path of the method call is being sent to.
     */
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)

    /*!
        \qmlproperty string SignalWatcher::iface

        The interface of the method call.
     */
    Q_PROPERTY(QString iface READ interface WRITE setInterface NOTIFY interfaceChanged)

public:
    explicit DBusSignalWatcher(QObject *parent = nullptr);

    bool enabled() const;
    void setEnabled(bool value);

    QVariant busType() const;
    void setBusType(const QVariant &type);

    QString service() const;
    void setService(const QString &value);

    QString path() const;
    void setPath(const QString &path);

    QString interface() const;
    void setInterface(const QString &iface);

Q_SIGNALS:
    void enabledChanged();
    void busTypeChanged();
    void serviceChanged();
    void pathChanged();
    void interfaceChanged();

private Q_SLOTS:
    void onReceivedSignal(const QDBusMessage &message);

private:
    void classBegin() override;
    void componentComplete() override;

    bool isValid() const;
    QDBusConnection connection() const;
    void connectToSignals();
    void disconnectFromSignals();

    bool m_enabled = true;
    std::variant<BusType::Type, QString> m_busType = BusType::Session;
    QString m_service;
    QString m_path;
    QString m_interface;
    QString m_connectionPrefix;

    bool m_ready = false;
};
}
