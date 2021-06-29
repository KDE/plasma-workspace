/*
    SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
    SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <solid/device.h>

class DeviceAction
{
public:
    DeviceAction();
    virtual ~DeviceAction();

    QString label() const;
    QString iconName() const;

    virtual QString id() const = 0;
    virtual void execute(Solid::Device &device) = 0;

protected:
    void setLabel(const QString &label);
    void setIconName(const QString &icon);

private:
    QString m_label;
    QString m_iconName;
};
