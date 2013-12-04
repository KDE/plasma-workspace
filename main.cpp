/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QApplication>
#include <qcommandlineparser.h>

#include <klocalizedstring.h>
#include "shellpluginloader.h"
#include "shellmanager.h"

#include <QtQml/QQmlDebuggingEnabler>
#include <QDebug>

static const char description[] = "Plasma Shell";
static const char version[] = "2.0";
static QCommandLineParser parser;

int main(int argc, char** argv)
{

    QApplication app(argc, argv);
    app.setApplicationVersion(version);

    QCommandLineOption dbg = QCommandLineOption(QStringList() << QStringLiteral("d") <<
                                                QStringLiteral("qmljsdebugger"),
                                                QStringLiteral("Enable QML Javascript debugger"));

    QCommandLineOption windowed = QCommandLineOption(QStringList() << QStringLiteral("w") <<
                                                     QStringLiteral("windowed"),
                                                     QStringLiteral("Force a windowed view for testing purposes"));

    QCommandLineOption crashesOption(QStringLiteral("crashes"),
                                     QStringLiteral("Recent number of crashes"),
                                     QStringLiteral("n"));

    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(description);
    parser.addOption(dbg);
    parser.addOption(windowed);
    parser.addOption(crashesOption);

    parser.process(app);

    //enable the QML debugger only if --qmljsdebugger (or -d) is passed as a command line arg
    //this must be called before the QApplication constructor
    if (parser.isSet(dbg)) {
        QQmlDebuggingEnabler enabler;
    }

    Plasma::PluginLoader::setPluginLoader(new ShellPluginLoader);

    ShellManager::setCrashCount(parser.value(crashesOption).toInt());
    ShellManager::s_forceWindowed = parser.isSet(windowed);
    ShellManager::instance();

    return app.exec();
}
