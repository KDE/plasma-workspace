/*
    SPDX-FileCopyrightText: 2011 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "splashapp.h"
#include <QQuickWindow>

#include <QTextStream>

#include <kworkspace.h>

#include <iostream>
#include <unistd.h>

void logMessageHandler(QtMsgType type, const char *msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QStringLiteral("Debug: %1").arg(msg);
        break;
    case QtInfoMsg:
        txt = QStringLiteral("Info: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QStringLiteral("Warning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt = QStringLiteral("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QStringLiteral("Fatal: %1").arg(msg);
        break;
    }
    QFile outFile(QStringLiteral("/tmp/ksplash"));
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << '\n';
}

int main(int argc, char **argv)
{
    bool test = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp("--test", argv[i]) == 0 || strcmp("--help", argv[i]) == 0)
            test = true;
    }
    // enable to send log output to /tmp/ksplash
    // which is useful for debugging
    //     qInstallMsgHandler(myMessageHandler);
    KWorkSpace::detectPlatform(argc, argv);
    QQuickWindow::setDefaultAlphaBuffer(true);
    SplashApp app(argc, argv);

    return app.exec();
}
