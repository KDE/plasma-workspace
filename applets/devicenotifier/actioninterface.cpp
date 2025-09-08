/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "actioninterface.h"

#include <QStandardPaths>
#include <devicenotifier_debug.h>

#include <KService>

#include <deviceserviceaction.h>
#include <qloggingcategory.h>

#include <Solid/Device>

ActionInterface::ActionInterface(const QString &udi, QObject *parent)
    : QObject(parent)
    , m_udi(udi)
{
}

ActionInterface::~ActionInterface() = default;

QString ActionInterface::predicate() const
{
    return {};
}

void ActionInterface::triggered()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Default action triggered: " << predicate();
    const QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"solid/actions/" + predicate());

    auto services = KService(filePath).actions();
    if (services.size() < 1) {
        qWarning() << "Failed to resolve hotplugjob action" << predicate() << filePath;
        return;
    }
    // Cannot be > 1, we only have one filePath, and < 1 was handled as error.
    Q_ASSERT(services.size() == 1);

    DeviceServiceAction action;
    action.setService(services.takeFirst());

    Solid::Device device(m_udi);
    action.execute(device);
}

bool ActionInterface::isValid() const
{
    qCWarning(APPLETS::DEVICENOTIFIER) << "Action: " << predicate() << " not valid";
    return false;
}

#include "moc_actioninterface.cpp"
