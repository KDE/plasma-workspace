/*
    SPDX-FileCopyrightText: 2011 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "splashapp.h"
#include <QQuickWindow>
#include <QSurfaceFormat>

#include <QFile>
#include <QTextStream>

#include <kworkspace.h>

int main(int argc, char **argv)
{
    // read ksplashrc as config file, not ksplashqmlrc
    QCoreApplication::setApplicationName(QStringLiteral("ksplash"));

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    KWorkSpace::detectPlatform(argc, argv);
    QQuickWindow::setDefaultAlphaBuffer(true);
    SplashApp app(argc, argv);

    return app.exec();
}
