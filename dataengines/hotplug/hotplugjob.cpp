/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2023 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "hotplugjob.h"

#include "deviceserviceaction.h"

#include <KLocalizedString>
#include <Solid/Device>

#include <QDebug>
#include <QStandardPaths>

void HotplugJob::start()
{
    if (operationName() == QLatin1String("invokeAction")) {
        const QString desktopFile = parameters()[QStringLiteral("predicate")].toString();
        const QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "solid/actions/" + desktopFile);

        DeviceServiceAction action;
        action.setDesktopFile(filePath);

        Solid::Device device(m_dest);
        action.execute(device);
    }

    emitResult();
}
