/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "actionscontrol.h"

#include "devicenotifier_debug.h"

#include <QStandardPaths>

// solid specific includes
#include <Solid/Predicate>

#include <Solid/OpticalDrive>

#include <KLocalizedString>
#include <KService>

#include <actions/defaultaction.h>
#include <actions/mountaction.h>
#include <actions/mountandopenaction.h>
#include <actions/openwithfilemanageraction.h>
#include <actions/unmountaction.h>

ActionsControl::ActionsControl(const QString &udi, QObject *parent)
    : QAbstractListModel(parent)
    , m_udi(udi)
    , m_predicatesMonitor(PredicatesMonitor::instance())

{
    m_defaultAction = new MountAndOpenAction{udi, this};
    m_unmountAction = new UnmountAction{m_udi, this};

    qCDebug(APPLETS::DEVICENOTIFIER) << "begin initializing Action Controller for device: " << m_udi << "; Default action: " << m_defaultAction->predicate();

    updateActionsForPredicates(m_predicatesMonitor->predicates());

    connect(m_predicatesMonitor.get(), &PredicatesMonitor::predicatesChanged, this, &ActionsControl::onPredicatesChanged);

    connect(m_unmountAction, &ActionInterface::isValidChanged, this, &ActionsControl::onIsActionValidChanged);

    connect(m_defaultAction, &ActionInterface::iconChanged, this, &ActionsControl::defaultActionIconChanged);
    connect(m_defaultAction, &ActionInterface::textChanged, this, &ActionsControl::defaultActionTextChanged);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << udi << " : Initializing complete";
}

ActionsControl::~ActionsControl()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for: " << m_udi << "was destroyed";
}

bool ActionsControl::isEmpty() const
{
    return m_isEmpty;
}

QString ActionsControl::defaultActionName() const
{
    return m_defaultAction->name();
}

QString ActionsControl::defaultActionIcon() const
{
    return m_defaultAction->icon();
}

QString ActionsControl::defaultActionText() const
{
    return m_defaultAction->text();
}

int ActionsControl::rowCount(const QModelIndex &parent) const
{
    return m_actions.size();
}

QVariant ActionsControl::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    switch (role) {
    case Name:
        return m_actions[index.row()]->name();
    case Icon:
        return m_actions[index.row()]->icon();
    case Text:
        return m_actions[index.row()]->text();
    }

    qCWarning(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                       << "Role not valid";
    return {};
}

QHash<int, QByteArray> ActionsControl::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Name] = "Name";
    roles[Icon] = "Icon";
    roles[Text] = "Text";
    return roles;
}

bool ActionsControl::isUnmountable() const
{
    return m_unmountAction->isValid();
}

void ActionsControl::unmount()
{
    m_unmountAction->triggered();
}

void ActionsControl::actionTriggered(const QString &name)
{
    if (m_defaultAction->name() == name) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                         << "Default action triggered";
        m_defaultAction->triggered();
        return;
    }

    for (auto action : m_actions) {
        if (action->name() == name) {
            action->triggered();
            return;
        }
    }
}

void ActionsControl::addActions()
{
    ActionInterface *action = new MountAction(m_udi, this);
    connect(action, &ActionInterface::isValidChanged, this, &ActionsControl::onIsActionValidChanged);
    if (action->isValid()) {
        m_actions.append(action);
    } else {
        m_notValidActions[action->name()] = std::make_pair(m_actions.size(), action);
    }

    action = new OpenWithFileManagerAction(m_udi, this);
    connect(action, &ActionInterface::isValidChanged, this, &ActionsControl::onIsActionValidChanged);
    if (action->isValid()) {
        m_actions.append(action);
    } else {
        m_notValidActions[action->name()] = std::make_pair(m_actions.size(), action);
    }

    for (auto action : m_actions) {
        connect(action, &ActionInterface::iconChanged, this, &ActionsControl::onActionIconChanged);
        connect(action, &ActionInterface::textChanged, this, &ActionsControl::onActionTextChanged);
        connect(action, &ActionInterface::isValidChanged, this, &ActionsControl::onIsActionValidChanged);
        qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : Custom action " << action->name() << "was added";
    }
}

bool ActionsControl::blockActions(const QString &action)
{
    if (action == QLatin1String("openWithFileManager.desktop")) {
        return false;
    }
    return true;
}

void ActionsControl::updateActionsForPredicates(const QHash<QString, Solid::Predicate> &predicates)
{
    if (!m_actions.isEmpty()) {
        for (auto action : m_actions) {
            action->deleteLater();
        }
        m_actions.clear();
    }

    addActions();

    m_isEmpty = true;

    Solid::Device device(m_udi);

    QStringList interestingDesktopFiles;
    // search in all desktop configuration file if the device inserted is a correct device
    QHashIterator it(predicates);
    // qDebug() << "=================" << udi;
    while (it.hasNext()) {
        it.next();
        if (it.value().matches(device)) {
            m_isEmpty = false;
            if (blockActions(it.key())) {
                interestingDesktopFiles << it.key();
            } else {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : " << it.key() << " action was blocked";
            }
        }
    }

    if (m_isEmpty) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                         << "Don't have default actions";
        return;
    }

    for (const QString &desktop : interestingDesktopFiles) {
        auto newAction = new DefaultAction(m_udi, desktop, this);
        if (newAction->isValid()) {
            m_actions.append(newAction);
            connect(newAction, &ActionInterface::iconChanged, this, &ActionsControl::onActionIconChanged);
            connect(newAction, &ActionInterface::textChanged, this, &ActionsControl::onActionTextChanged);
            connect(newAction, &ActionInterface::isValidChanged, this, &ActionsControl::onIsActionValidChanged);
            qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                             << " action " << desktop << " added";
        }
    }
}

void ActionsControl::onPredicatesChanged(const QHash<QString, Solid::Predicate> &predicates)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                     << "predicatesChanged signal arrived. Begin resetting model";
    beginResetModel();
    updateActionsForPredicates(predicates);
    endResetModel();
    qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                     << "resetting ended";
}

void ActionsControl::onIsActionValidChanged(const QString &name, bool status)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                     << "isActionValidChanged signal arrived for action " << name << " with status " << status;
    if (name == QStringLiteral("Unmount")) {
        Q_EMIT unmountActionIsValidChanged(m_udi, status);
    } else if (status) {
        if (auto it = m_notValidActions.find(name); it != m_notValidActions.end()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                             << "adding new action " << name;
            beginInsertRows(QModelIndex(), 0, 0);
            m_actions.prepend(it->second);
            m_notValidActions.remove(name);
            endInsertRows();
        }
    } else {
        for (int position = 0; position < m_actions.size(); ++position) {
            if (m_actions[position]->name() == name) {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                                 << "remove action " << name;
                beginRemoveRows(QModelIndex(), position, position);
                m_notValidActions[m_actions[position]->name()] = std::make_pair(position, m_actions[position]);
                m_actions.removeAt(position);
                endRemoveRows();
                return;
            }
        }
    }
}

void ActionsControl::onActionIconChanged(const QString &action)
{
    if (m_defaultAction->name() == action) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                         << "Icon for default action changed";
        Q_EMIT defaultActionIconChanged(m_defaultAction->icon());
    } else {
        for (int position = 0; position < m_actions.size(); ++position) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                             << "Icon for " << action << " changed";
            QModelIndex index = ActionsControl::index(position);
            Q_EMIT dataChanged(index, index, {Icon});
        }
    }
}

void ActionsControl::onActionTextChanged(const QString &action)
{
    if (m_defaultAction->name() == action) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                         << "Text for default action changed";
        Q_EMIT defaultActionTextChanged(m_defaultAction->text());
    } else {
        for (int position = 0; position < m_actions.size(); ++position) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Action Controller for " << m_udi << " : "
                                             << "Text for " << action << " changed";
            QModelIndex index = ActionsControl::index(position);
            Q_EMIT dataChanged(index, index, {Text});
        }
    }
}

#include "moc_actionscontrol.cpp"
