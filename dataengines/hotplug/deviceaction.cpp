/*
    SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
    SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "deviceaction.h"

DeviceAction::DeviceAction()
{
}

DeviceAction::~DeviceAction()
{
}

void DeviceAction::setIconName(const QString &iconName)
{
    m_iconName = iconName;
}

void DeviceAction::setLabel(const QString &label)
{
    m_label = label;
}

QString DeviceAction::iconName() const
{
    return m_iconName;
}

QString DeviceAction::label() const
{
    return m_label;
}
