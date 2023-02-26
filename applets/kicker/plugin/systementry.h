/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractentry.h"
#include <sessionmanagement.h>

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

    /**
     * Deserialized map of arguments for the run() method.
     */
    struct Arguments {
        /**
         * Construct Arguments object with default values.
         */
        explicit Arguments();

        /**
         * Parse arguments from a variant map.
         *
         * The given map shall contain at most one key for now: "confirmationMode".
         * In future it is possible to add more keys if needed.
         */
        explicit Arguments(const QVariant &argument);

        /**
         * Logout, Reboot and Shutdown actions take confirmation mode argument.
         * Default mode is SessionManagement::ConfirmationMode::Default.
         */
        SessionManagement::ConfirmationMode confirmationMode;
    };

    /**
     * Run this entry with given arguments.
     *
     * The difference from run(const QString &, const QVariant &) overload is
     * that arguments map is already parsed.
     */
    bool run(const QString &actionId, const Arguments &argument);

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
