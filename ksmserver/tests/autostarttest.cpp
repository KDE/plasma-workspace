/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

#include "../autostart.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("phase"), QStringLiteral("autostart phase number"));
    parser.process(app);

    QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        parser.showHelp(1);
    }

    int phase = args.first().toInt();
    AutoStart autostart;
    autostart.setPhase(phase);
    autostart.loadAutoStartList();

    QTextStream out(stdout);

    while (!autostart.phaseDone()) {
        QString service = autostart.startService();
        if (service.isEmpty()) {
            break;
        }

        out << service << "\n";
    }

    return 0;
}
