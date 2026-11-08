/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-workspace.h>

#include "xsystemsettings.h"

#include <QCoreApplication>
#include <QProcess>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    XSystemSettings::sync();

    QProcess x11KdedProcess;
    x11KdedProcess.start(QStringLiteral(PLASMA_KDED_X11_PATH));
    x11KdedProcess.waitForFinished(); // it will fork itself
    return 0;
}
