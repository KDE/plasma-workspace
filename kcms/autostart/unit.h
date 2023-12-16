/*
    SPDX-FileCopyrightText: 2023 Thenujan Sandramohan <sthenujan2002@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>

class QDBusPendingCallWatcher;

class Unit : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString activeState MEMBER m_activeState NOTIFY dataChanged);
    Q_PROPERTY(QString activeStateValue MEMBER m_activeStateValue NOTIFY dataChanged);
    Q_PROPERTY(QString description MEMBER m_description NOTIFY dataChanged);
    Q_PROPERTY(QString timeActivated MEMBER m_timeActivated NOTIFY dataChanged);
    Q_PROPERTY(QString logs MEMBER m_logs NOTIFY dataChanged);
    Q_PROPERTY(bool invalid MEMBER m_invalid NOTIFY dataChanged);

public:
    explicit Unit(QObject *parent = nullptr, bool invalid = false);
    ~Unit() override;

    void setId(const QString &id);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reloadLogs();

Q_SIGNALS:
    void journalError(const QString &message);
    void error(const QString &message);
    void dataChanged();

private Q_SLOTS:
    void dbusPropertiesChanged(QString name, QVariantMap map, QStringList list);
    void callFinishedSlot(QDBusPendingCallWatcher *call);

private:
    void loadAllProperties();
    void setActiveEnterTimestamp(qulonglong ActiveEnterTimestamp);
    void getAllCallback(QDBusPendingCallWatcher *call);
    QStringList getLastJournalEntries(const QString &unit);

    QString m_id;
    QString m_description;
    QString m_activeState;
    QString m_activeStateValue;
    QString m_timeActivated;
    QString m_logs;
    QDBusObjectPath m_dbusObjectPath;
    bool m_invalid;

    const QString m_connSystemd = QStringLiteral("org.freedesktop.systemd1");
    const QString m_pathSysdMgr = QStringLiteral("/org/freedesktop/systemd1");
    const QString m_ifaceMgr = QStringLiteral("org.freedesktop.systemd1.Manager");
    const QString m_ifaceUnit = QStringLiteral("org.freedesktop.systemd1.Unit");
    QDBusConnection m_sessionBus = QDBusConnection::sessionBus();
};

Q_DECLARE_METATYPE(Unit);
