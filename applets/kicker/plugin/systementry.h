/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractentry.h"

#include <QObject>

class SessionManagement;

class SystemEntry : public QObject, public AbstractEntry
{
    Q_OBJECT

public:
    enum Action {
        NoAction = 0,
        LockSession,
        LogoutSession,
        SaveSession,
        SwitchUser,
        Suspend,
        Hibernate,
        Reboot,
        Shutdown,
    };

    explicit SystemEntry(AbstractModel *owner, Action action);
    explicit SystemEntry(AbstractModel *owner, const QString &id);
    ~SystemEntry();

    Action action() const;

    EntryType type() const override
    {
        return RunnableType;
    }

    bool isValid() const override;

    QIcon icon() const override;
    QString iconName() const;
    QString name() const override;
    QString group() const override;
    QString description() const override;

    QString id() const override;

    bool run(const QString &actionId = QString(), const QVariant &argument = QVariant()) override;

Q_SIGNALS:
    void isValidChanged() const;
    void sessionManagementStateChanged();

private Q_SLOTS:
    void refresh();

private:
    bool m_initialized;

    Action m_action;
    bool m_valid;

    static int s_instanceCount;
    static SessionManagement *s_sessionManagement;
};
