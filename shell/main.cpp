/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
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
#include <QtQml/QQmlDebuggingEnabler>
#include <QDebug>

#include <kdbusservice.h>
#include <klocalizedstring.h>

#include "shellpluginloader.h"
#include "shellmanager.h"

static const char description[] = "Plasma Shell";
static const char version[] = "2.0";
static QCommandLineParser parser;

void noMessageOutput(QtMsgType type, const char *msg)
{
     Q_UNUSED(type);
     Q_UNUSED(msg);
}
int main(int argc, char** argv)
{
    QQmlDebuggingEnabler debugEnabler;

    QApplication app(argc, argv);
    app.setApplicationName("plasma_shell");
    app.setOrganizationDomain("kde.org");
    app.setApplicationVersion(version);
    app.setQuitOnLastWindowClosed(false);
    parser.setApplicationDescription(description);
    KDBusService service(KDBusService::Unique);

    QCommandLineOption dbg(QStringList() << QStringLiteral("d") <<
                           QStringLiteral("qmljsdebugger"),
                           QStringLiteral("Enable QML Javascript debugger"));

    QCommandLineOption win(QStringList() << QStringLiteral("w") <<
                           QStringLiteral("windowed"),
                           QStringLiteral("Force a windowed view for testing purposes"));

    QCommandLineOption respawn(QStringList() << QStringLiteral("n") <<
                           QStringLiteral("no-respawn"),
                           QStringLiteral("Do not restart plasma-shell automatically after a crash"));

    QCommandLineOption crash(QStringList() << QStringLiteral("c") << QStringLiteral("crashes"),
                                     QStringLiteral("Recent number of crashes"),
                                     QStringLiteral("n"));

    QCommandLineOption shutup(QStringList() << QStringLiteral("s") << QStringLiteral("shut-up"),
                                     QStringLiteral("Shuts up the output"));

    QCommandLineOption shellPlugin(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                     QStringLiteral("Force loading the given shell plugin"),
                                     QStringLiteral("plugin"));

    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(dbg);
    parser.addOption(win);
    parser.addOption(respawn);
    parser.addOption(crash);
    parser.addOption(shutup);
    parser.addOption(shellPlugin);

    parser.process(app);

    if (parser.isSet(shutup)) {
        qInstallMsgHandler(noMessageOutput);
    }
    Plasma::PluginLoader::setPluginLoader(new ShellPluginLoader);

    ShellManager::setCrashCount(parser.value(crash).toInt());
    ShellManager::s_forceWindowed = parser.isSet(win);
    ShellManager::s_noRespawn = parser.isSet(respawn);
    ShellManager::s_fixedShell = parser.value(shellPlugin);
    ShellManager::instance();

    return app.exec();
}
