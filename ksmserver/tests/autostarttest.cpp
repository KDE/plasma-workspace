/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2014 Vishesh Handa <me@vhanda.in>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

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
