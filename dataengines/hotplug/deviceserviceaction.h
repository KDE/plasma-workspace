/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DEVICESERVICEACTION_H
#define DEVICESERVICEACTION_H

#include "deviceaction.h"

#include <kserviceaction.h>
#include <solid/predicate.h>

class DeviceServiceAction : public DeviceAction
{
public:
    DeviceServiceAction();
    QString id() const override;
    void execute(Solid::Device &device) override;

    void setService(const KServiceAction &service);
    KServiceAction service() const;

private:
    KServiceAction m_service;
};

#endif
