/*
 * SPDX-FileCopyrightText: 2025 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "checkaction.h"

#include "devicenotifier_debug.h"

#include <KLocalizedString>

#include "devicestatemonitor_p.h"

CheckAction::CheckAction(const QString &udi, QObject *parent)
    : ActionInterface(udi, parent)
    , m_stateMonitor(DevicesStateMonitor::instance())
{
    Solid::Device device(udi);

    // It's possible for there to be no StorageAccess (e.g. MTP devices don't have one)
    if (device.is<Solid::StorageAccess>()) {
        auto *access = device.as<Solid::StorageAccess>();
        if (access) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Check action: have storage access";
            connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &CheckAction::updateIsValid);
        }
    }
}

CheckAction::~CheckAction()
{
}

void CheckAction::triggered()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Check Action: Triggered! Begin checking";

    Solid::Device device(m_udi);

    if (device.is<Solid::StorageAccess>()) {
        auto *access = device.as<Solid::StorageAccess>();
        if (access && access->canCheck()) {
            access->check();
        }
    }
}

bool CheckAction::isValid() const
{
    Solid::Device device(m_udi);

    if (device.is<Solid::StorageAccess>()) {
        auto *access = device.as<Solid::StorageAccess>();
        if (access && access->canCheck() && !access->isAccessible() && !m_stateMonitor->isChecked(m_udi)) {
            return true;
        }
    }
    return false;
}

QString CheckAction::name() const
{
    return QStringLiteral("Check");
}

QString CheckAction::icon() const
{
    return QStringLiteral("tools-wizard");
}

QString CheckAction::text() const
{
    return i18nc("@action:button check a storageAccess (i.e disk) for error", "Check for Errors");
}

void CheckAction::updateIsValid(const QString &udi)
{
    if (udi == m_udi) {
        Q_EMIT isValidChanged(name(), isValid());
    }
}
