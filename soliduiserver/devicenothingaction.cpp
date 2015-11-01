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

#include "devicenothingaction.h"

#include <KLocalizedString>

DeviceNothingAction::DeviceNothingAction()
	: DeviceAction()
{
    setIconName(QStringLiteral("dialog-cancel"));
    setLabel(i18n("Do nothing"));
}

QString DeviceNothingAction::id() const
{
    return QStringLiteral("#NothingAction");
}

void DeviceNothingAction::execute(Solid::Device &/*device*/)
{
}
