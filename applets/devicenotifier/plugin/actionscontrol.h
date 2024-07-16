/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QAbstractListModel>

#include <qqmlregistration.h>

#include "actioninterface.h"
#include "predicatesmonitor_p.h"

class ActionsControl : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Non-createable type that is associated with a specific device")

    Q_PROPERTY(QString defaultActionName READ defaultActionName CONSTANT)
    Q_PROPERTY(QString defaultActionIcon READ defaultActionIcon NOTIFY defaultActionIconChanged)
    Q_PROPERTY(QString defaultActionText READ defaultActionText NOTIFY defaultActionTextChanged)

public:
    enum ActionInfo {
        Name = Qt::UserRole + 1,
        Icon,
        Text,
    };

    Q_ENUM(ActionInfo)

    Q_INVOKABLE void actionTriggered(const QString &operation);

    explicit ActionsControl(const QString &udi, QObject *parent = nullptr);
    ~ActionsControl() override;

    bool isEmpty() const;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isUnmountable() const;
    void unmount();

private:
    QString defaultActionName() const;
    QString defaultActionIcon() const;
    QString defaultActionText() const;

    void updateActionsForPredicates(const QHash<QString, Solid::Predicate> &predicates);

    void addActions();
    bool blockActions(const QString &action);

Q_SIGNALS:
    void defaultActionIconChanged(const QString &icon);
    void defaultActionTextChanged(const QString &text);

    void unmountActionIsValidChanged(const QString &udi, bool status);

private Q_SLOTS:
    void onPredicatesChanged(const QHash<QString, Solid::Predicate> &predicates);

    void onIsActionValidChanged(const QString &action, bool status);
    void onActionIconChanged(const QString &action);
    void onActionTextChanged(const QString &action);

private:
    QString m_udi;
    bool m_isEmpty;
    ActionInterface *m_defaultAction;
    ActionInterface *m_unmountAction;
    QList<ActionInterface *> m_actions;
    QHash<QString, std::pair<int, ActionInterface *>> m_notValidActions;

    std::shared_ptr<PredicatesMonitor> m_predicatesMonitor;
};
