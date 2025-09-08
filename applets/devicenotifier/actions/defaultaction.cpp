/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "defaultaction.h"

#include "devicenotifier_debug.h"

#include <QStandardPaths>

#include <KService>

DefaultAction::DefaultAction(const QString &udi, const QString &desktopFile, QObject *parent)
    : ActionInterface(udi, parent)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Initializing default action with " << desktopFile << " predicate";
    const QString actionUrl = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"solid/actions/" + desktopFile);
    auto services = KService(actionUrl).actions();
    m_isValid = !services.isEmpty();
    if (m_isValid) {
        m_predicate = desktopFile;
        m_text = services[0].text();
        m_icon = services[0].icon();
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "action " << desktopFile << " : not valid";
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "default action with " << desktopFile << " predicate successfully initialized";
}

DefaultAction::~DefaultAction() = default;

QString DefaultAction::name() const
{
    return predicate();
}

QString DefaultAction::predicate() const
{
    return m_predicate;
}

bool DefaultAction::isValid() const
{
    return m_isValid;
}

QString DefaultAction::icon() const
{
    return m_icon;
}

QString DefaultAction::text() const
{
    return m_text;
}

#include "moc_defaultaction.cpp"
