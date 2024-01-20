/*
    SPDX-FileCopyrightText: 2010 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 Lukáš Tinkl <ltinkl@redhat.com>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <qqmlregistration.h>

#include <Solid/Device>
#include <solid/solidnamespace.h>

/**
 * @brief Class which triggers solid notifications
 *
 * This is an internal class which listens to solid errors and route them via dbus to an
 * appropriate visualization (e.g. the plasma device notifier applet); if such visualization is not available
 * errors are shown via regular notifications
 *
 * @author Jacopo De Simoi <wilderkde at gmail.com>
 */

class KSolidNotify : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString lastUdi READ lastUdi NOTIFY lastUdiChanged)

    // Error type
    Q_PROPERTY(Solid::ErrorType lastErrorType READ lastErrorType NOTIFY lastErrorTypeChanged)

    // Error title
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

    // Device description
    Q_PROPERTY(QString lastDescription READ lastDescription NOTIFY lastDescriptionChanged)

    // Error icon
    Q_PROPERTY(QString lastIcon READ lastIcon NOTIFY lastIconChanged)

public:
    explicit KSolidNotify(QObject *parent = nullptr);

    QString lastUdi() const;
    Solid::ErrorType lastErrorType() const;
    QString lastMessage() const;
    QString lastDescription() const;
    QString lastIcon() const;

    Q_INVOKABLE void clearMessage();

Q_SIGNALS:
    void lastUdiChanged();
    void lastErrorTypeChanged();
    void lastMessageChanged();
    void lastDescriptionChanged();
    void lastIconChanged();

    void blockingAppsReady(const QMap<unsigned, QString> &apps);

private Q_SLOTS:
    void onDeviceAdded(const QString &udi);
    void onDeviceRemoved(const QString &udi);

private:
    enum class SolidReplyType {
        Setup,
        Teardown,
        Eject,
    };

    void onSolidReply(SolidReplyType type, Solid::ErrorType error, const QVariant &errorData, const QString &udi);
    void notify(Solid::ErrorType error, const QString &errorMessage, const QString &errorData, const QString &udi, const QString &icon);

    void connectSignals(Solid::Device &device);
    bool isSafelyRemovable(const QString &udi) const;
    void queryBlockingApps(const QString &devicePath);

    QHash<QString, Solid::Device> m_devices;
    Solid::ErrorType m_lastErrorType = static_cast<Solid::ErrorType>(0);
    QString m_lastUdi;
    QString m_lastMessage;
    QString m_lastDescription;
    QString m_lastIcon;
};
