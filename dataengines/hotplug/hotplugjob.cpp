/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "hotplugjob.h"

#include "deviceserviceaction.h"

#include <KDesktopFileActions>
#include <KLocalizedString>
#include <KService>
#include <Solid/Device>

#include <QDebug>
#include <QStandardPaths>

void HotplugJob::start()
{
    if (operationName() == QLatin1String("invokeAction")) {
        const QString desktopFile = parameters()[QStringLiteral("predicate")].toString();
        const QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "solid/actions/" + desktopFile);

        QList<KServiceAction> services = KDesktopFileActions::userDefinedServices(KService(filePath), true);
        if (services.size() < 1) {
            qWarning() << "Failed to resolve hotplugjob action" << desktopFile << filePath;
            setError(KJob::UserDefinedError);
            setErrorText(i18nc("error; %1 is the desktop file name of the service", "Failed to resolve service action for %1.", desktopFile));
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
