/*
 *   Copyright (C) 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright (C) 2019 Harald Sitter <sitter@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "hotplugjob.h"
#include "hotplugengine.h"

#include "deviceserviceaction.h"

#include <QDebug>
#include <QStandardPaths>
#include <KDesktopFileActions>
#include <KLocalizedString>

void HotplugJob::start()
{
    if (operationName() == QLatin1String("invokeAction")) {
        const QString desktopFile = parameters()[QStringLiteral("predicate")].toString();
        const QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "solid/actions/" + desktopFile);

        QList<KServiceAction> services = KDesktopFileActions::userDefinedServices(filePath, true);
        if (services.size() < 1) {
            qWarning() << "Failed to resolve hotplugjob action" << desktopFile << filePath;
            setError(KJob::UserDefinedError);
            setErrorText(i18nc("error; %1 is the desktop file name of the service",
                               "Failed to resolve service action for %1.", desktopFile));
            setResult(false); // calls emitResult internally.
            return;
        }
        // Cannot be > 1, we only have one filePath, and < 1 was handled as error.
        Q_ASSERT(services.size() == 1);

        DeviceServiceAction action;
        action.setService(services.takeFirst());

        Solid::Device device(m_dest);
        action.execute(device);
    }

    emitResult();
}



