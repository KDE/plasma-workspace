/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

#include <QApplication>

#include "statusnotifiertest.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCommandLineParser parser;

    const QString description = QStringLiteral("Statusnotifier test app");
    const char version[] = "1.0";

    app.setApplicationVersion(version);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(description);

    StatusNotifierTest test;
    int ex = test.runMain();
    app.exec();
    return ex;
}
