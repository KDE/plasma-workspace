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
    DeviceServiceAction(const KServiceAction &service);
    void execute(Solid::Device &device);

private:
    KServiceAction m_service;
};
