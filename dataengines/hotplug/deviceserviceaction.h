/*
    SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
    SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>
    SPDX-FileCopyrightText: 2023 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KDesktopFile>
#include <solid/predicate.h>

class DeviceServiceAction
{
public:
    void execute(Solid::Device &device);

    void setDesktopFile(const QString &filePath);
    KDesktopFile *desktopFile() const;

private:
    KDesktopFile *m_desktopFile;
};
