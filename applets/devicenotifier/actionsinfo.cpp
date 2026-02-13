/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "actionsinfo.h"

#include "devicenotifier_debug.h"

#include <QStandardPaths>

// solid specific includes
#include <Solid/Predicate>

#include <Solid/OpticalDrive>

#include <KLocalizedString>
#include <KService>

#include <actions/checkaction.h>
#include <actions/defaultaction.h>
#include <actions/ejectaction.h>
#include <actions/mountaction.h>
#include <actions/mountandopenaction.h>
#include <actions/openwithfilemanageraction.h>
#include <actions/unmountaction.h>

#include "predicatesmonitor_p.h"

ActionsInfo::ActionsInfo(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent)
    : QAbstractListModel(parent)
    , m_storageInfo(storageInfo)
    , m_stateInfo(stateInfo)
    , m_predicatesMonitor(PredicatesMonitor::instance())

{
    m_defaultAction = new MountAndOpenAction{m_storageInfo, stateInfo, this};
    m_ejectAction = new EjectAction{m_storageInfo, stateInfo, this};

    qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : Begin initializing";

    updateActionsForPredicates(m_predicatesMonitor->predicates());

    connect(m_predicatesMonitor.get(), &PredicatesMonitor::predicatesChanged, this, &ActionsInfo::onPredicatesChanged);

    connect(m_ejectAction, &ActionInterface::isValidChanged, this, &ActionsInfo::onIsActionValidChanged);

    connect(m_defaultAction, &ActionInterface::iconChanged, this, &ActionsInfo::defaultActionIconChanged);
    connect(m_defaultAction, &ActionInterface::textChanged, this, &ActionsInfo::defaultActionTextChanged);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : Initializing complete";
}

ActionsInfo::~ActionsInfo()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : destroyed";
}

bool ActionsInfo::isEmpty() const
{
    return m_isEmpty;
}

QString ActionsInfo::defaultActionName() const
{
    return m_defaultAction->name();
}

QString ActionsInfo::defaultActionIcon() const
{
    return m_defaultAction->icon();
}

QString ActionsInfo::defaultActionText() const
{
    return m_defaultAction->text();
}

int ActionsInfo::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_actions.size();
}

QVariant ActionsInfo::data(const QModelIndex &index, int role) const
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

    qCWarning(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : Role not valid";
    return {};
}

QHash<int, QByteArray> ActionsInfo::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Name] = QByteArrayLiteral("Name");
    roles[Icon] = QByteArrayLiteral("Icon");
    roles[Text] = QByteArrayLiteral("Text");
    return roles;
}

bool ActionsInfo::isEjectable() const
{
    return m_ejectAction->isValid();
}

void ActionsInfo::eject()
{
    m_ejectAction->triggered();
}

void ActionsInfo::actionTriggered(const QString &name)
{
    if (m_defaultAction->name() == name) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : Default action triggered";
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

void ActionsInfo::addActions()
{
    ActionInterface *action = new UnmountAction(m_storageInfo, m_stateInfo, this);
    connect(action, &ActionInterface::isValidChanged, this, &ActionsInfo::onIsActionValidChanged);
    if (action->isValid()) {
        m_actions.append(action);
    } else {
        m_notValidActions[action->name()] = std::make_pair(m_actions.size(), action);
    }

    action = new CheckAction(m_storageInfo, m_stateInfo, this);
    connect(action, &ActionInterface::isValidChanged, this, &ActionsInfo::onIsActionValidChanged);
    if (action->isValid()) {
        m_actions.append(action);
    } else {
        m_notValidActions[action->name()] = std::make_pair(m_actions.size(), action);
    }

    action = new MountAction(m_storageInfo, m_stateInfo, this);
    connect(action, &ActionInterface::isValidChanged, this, &ActionsInfo::onIsActionValidChanged);
    if (action->isValid()) {
        m_actions.append(action);
    } else {
        m_notValidActions[action->name()] = std::make_pair(m_actions.size(), action);
    }

    action = new OpenWithFileManagerAction(m_storageInfo, m_stateInfo, this);
    connect(action, &ActionInterface::isValidChanged, this, &ActionsInfo::onIsActionValidChanged);
    if (action->isValid()) {
        m_actions.append(action);
    } else {
        m_notValidActions[action->name()] = std::make_pair(m_actions.size(), action);
    }

    for (auto action : m_actions) {
        connect(action, &ActionInterface::iconChanged, this, &ActionsInfo::onActionIconChanged);
        connect(action, &ActionInterface::textChanged, this, &ActionsInfo::onActionTextChanged);
        connect(action, &ActionInterface::isValidChanged, this, &ActionsInfo::onIsActionValidChanged);
        qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : Custom action " << action->name() << "was added";
    }
}

bool ActionsInfo::blockActions(const QString &action)
{
    if (action == QLatin1String("openWithFileManager.desktop")) {
        return false;
    }

    if (action == QLatin1String("solid_mtp.desktop")) {
        return false;
    }

    if (action == QLatin1String("solid_afc.desktop")) {
        return false;
    }

    if (action == QLatin1String("solid_camera.desktop")) {
        return false;
    }

    return true;
}

void ActionsInfo::updateActionsForPredicates(const QHash<QString, Solid::Predicate> &predicates)
{
    if (!m_actions.isEmpty()) {
        for (auto action : m_actions) {
            action->deleteLater();
        }
        m_actions.clear();
    }

    addActions();

    m_isEmpty = true;

    const Solid::Device &device = m_storageInfo->device();

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
                qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << device.udi() << " : " << it.key() << " action was blocked";
            }
        }
    }

    if (m_isEmpty) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << device.udi() << " : "
                                         << "Don't have default actions";
        return;
    }

    for (const QString &desktop : interestingDesktopFiles) {
        auto newAction = new DefaultAction(m_storageInfo, m_stateInfo, desktop, this);
        if (newAction->isValid()) {
            m_actions.append(newAction);
            connect(newAction, &ActionInterface::iconChanged, this, &ActionsInfo::onActionIconChanged);
            connect(newAction, &ActionInterface::textChanged, this, &ActionsInfo::onActionTextChanged);
            connect(newAction, &ActionInterface::isValidChanged, this, &ActionsInfo::onIsActionValidChanged);
            qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << device.udi() << " : "
                                             << " action " << desktop << " added";
        }
    }
}

void ActionsInfo::onPredicatesChanged(const QHash<QString, Solid::Predicate> &predicates)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                     << "predicatesChanged signal arrived. Begin resetting model";
    beginResetModel();
    updateActionsForPredicates(predicates);
    endResetModel();
    qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                     << "resetting ended";
}

void ActionsInfo::onIsActionValidChanged(const QString &name, bool status)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                     << "isActionValidChanged signal arrived for action " << name << " with status " << status;
    if (name == u"Eject") {
        Q_EMIT ejectActionIsValidChanged(m_storageInfo->device().udi(), status);
    } else if (status) {
        if (auto it = m_notValidActions.find(name); it != m_notValidActions.end()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                             << "adding new action " << name;
            beginInsertRows(QModelIndex(), 0, 0);
            m_actions.prepend(it->second);
            m_notValidActions.remove(name);
            endInsertRows();
        }
    } else {
        for (int position = 0; position < m_actions.size(); ++position) {
            if (m_actions[position]->name() == name) {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
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

void ActionsInfo::onActionIconChanged(const QString &action)
{
    if (m_defaultAction->name() == action) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                         << "Icon for default action changed";
        Q_EMIT defaultActionIconChanged(m_defaultAction->icon());
    } else {
        for (int position = 0; position < m_actions.size(); ++position) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                             << "Icon for " << action << " changed";
            QModelIndex index = ActionsInfo::index(position);
            Q_EMIT dataChanged(index, index, {Icon});
        }
    }
}

void ActionsInfo::onActionTextChanged(const QString &action)
{
    if (m_defaultAction->name() == action) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                         << "Text for default action changed";
        Q_EMIT defaultActionTextChanged(m_defaultAction->text());
    } else {
        for (int position = 0; position < m_actions.size(); ++position) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Actions Info " << m_storageInfo->device().udi() << " : "
                                             << "Text for " << action << " changed";
            QModelIndex index = ActionsInfo::index(position);
            Q_EMIT dataChanged(index, index, {Text});
        }
    }
}

#include "moc_actionsinfo.cpp"
