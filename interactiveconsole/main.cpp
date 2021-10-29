/*
    SPDX-FileCopyrightText: 2021 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "interactiveconsole.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    InteractiveConsole::ConsoleMode mode = InteractiveConsole::PlasmaConsole;

    QCommandLineParser parser;
    QCommandLineOption plasmaOpt(QStringLiteral("plasma"));
    QCommandLineOption kwinOpt(QStringLiteral("kwin"));
    parser.addOption(plasmaOpt);
    parser.addOption(kwinOpt);
    parser.addHelpOption();
    parser.process(app);
    if (parser.isSet(plasmaOpt) && parser.isSet(kwinOpt)) {
        qWarning() << "Only one mode can be specified when launching the interactive console";
        exit(1);
    } else if (parser.isSet(kwinOpt)) {
        mode = InteractiveConsole::KWinConsole;
    } else if (parser.isSet(plasmaOpt)) {
        mode = InteractiveConsole::PlasmaConsole;
    }
    // set to delete on close
    auto console = new InteractiveConsole(mode);
    console->show();
    app.exec();
}
