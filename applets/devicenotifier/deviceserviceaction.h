/*
    SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
    SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <kserviceaction.h>
#include <solid/predicate.h>

class DeviceServiceAction
{
public:
    void execute(Solid::Device &device);

    void setService(const KServiceAction &service);
    KServiceAction service() const;

private:
    KServiceAction m_service;
};
