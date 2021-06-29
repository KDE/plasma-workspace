/*
    SPDX-FileCopyrightText: 2012 Gregor Taetzner <gregor@freenet.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "packagekitjob.h"
#include <QDBusConnection>
#include <QDBusMessage>

PackagekitJob::PackagekitJob(const QString &destination, const QString &operation, const QMap<QString, QVariant> &parameters, QObject *parent)
    : ServiceJob(destination, operation, parameters, parent)
{
}

PackagekitJob::~PackagekitJob()
{
}

void PackagekitJob::start()
{
    const QString operation = operationName();

    if (operation == QLatin1String("uninstallApplication")) {
        QStringList files(parameters()[QStringLiteral("Url")].toString());
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PackageKit"),
                                                              QStringLiteral("/org/freedesktop/PackageKit"),
                                                              QStringLiteral("org.freedesktop.PackageKit.Modify"),
                                                              QStringLiteral("RemovePackageByFiles"));
        message << (uint)0;
        message << files;
        message << QString();

        QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
        setResult(true);
        return;
    }

    setResult(false);
}
