/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "openwithfilemanageraction.h"

#include <Solid/OpticalDrive>

#include <KService>

#include <QStandardPaths>

OpenWithFileManagerAction::OpenWithFileManagerAction(const QString &udi, QObject *parent)
    : ActionInterface(udi, parent)
    , m_stateMonitor(DevicesStateMonitor::instance())
{
    Solid::Device device(udi);

    const QString actionUrl = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"solid/actions/" + predicate());
    auto services = KService(actionUrl).actions();
    if (services.isEmpty()) {
        m_isActionValid = false;
        return;
    }
    m_text = services[0].text();
    m_icon = services[0].icon();

    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &OpenWithFileManagerAction::updateIsValid);

    m_isActionValid = true;
}

OpenWithFileManagerAction::~OpenWithFileManagerAction() = default;

QString OpenWithFileManagerAction::predicate() const
{
    return QLatin1String("openWithFileManager.desktop");
}

bool OpenWithFileManagerAction::isValid() const
{
    return m_isActionValid && m_stateMonitor->isRemovable(m_udi) && m_stateMonitor->isMounted(m_udi);
}

QString OpenWithFileManagerAction::name() const
{
    return QStringLiteral("openWithFileManager");
}

QString OpenWithFileManagerAction::icon() const
{
    return m_icon;
}

QString OpenWithFileManagerAction::text() const
{
    return m_text;
}

void OpenWithFileManagerAction::updateIsValid(const QString &udi)
{
    if (udi != m_udi) {
        return;
    }
    Q_EMIT isValidChanged(name(), isValid());
}

#include "moc_openwithfilemanageraction.cpp"
