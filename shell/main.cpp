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
#include <QQuickWindow>
#include <QSessionManager>
#include <QDebug>

#include <kdbusservice.h>
#include <klocalizedstring.h>

#include "shellpluginloader.h"
#include "shellmanager.h"

static const char description[] = "Plasma Shell";
static const char version[] = "4.96.0";
static QCommandLineParser parser;

void noMessageOutput(QtMsgType type, const char *msg)
{
     Q_UNUSED(type);
     Q_UNUSED(msg);
}
int main(int argc, char** argv)
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    KLocalizedString::setApplicationDomain("plasmashell");

    QApplication app(argc, argv);
    app.setApplicationName("plasmashell");
    app.setApplicationDisplayName(i18n("Plasma"));
    app.setOrganizationDomain("kde.org");
    app.setApplicationVersion(version);
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon::fromTheme("plasma"));
    parser.setApplicationDescription(description);
    KDBusService service(KDBusService::Unique);

    QCommandLineOption dbg(QStringList() << QStringLiteral("d") <<
                           QStringLiteral("qmljsdebugger"),
                           i18n("Enable QML Javascript debugger"));

    QCommandLineOption win(QStringList() << QStringLiteral("w") <<
                           QStringLiteral("windowed"),
                           i18n("Force a windowed view for testing purposes"));

    QCommandLineOption respawn(QStringList() << QStringLiteral("n") <<
                           QStringLiteral("no-respawn"),
                           i18n("Do not restart plasma-shell automatically after a crash"));

    QCommandLineOption crash(QStringList() << QStringLiteral("c") << QStringLiteral("crashes"),
                                     i18n("Recent number of crashes"),
                                     QStringLiteral("n"));

    QCommandLineOption shutup(QStringList() << QStringLiteral("s") << QStringLiteral("shut-up"),
                                     i18n("Shuts up the output"));

    QCommandLineOption shellPlugin(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                     i18n("Force loading the given shell plugin"),
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

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    Plasma::PluginLoader::setPluginLoader(new ShellPluginLoader);

    ShellManager::setCrashCount(parser.value(crash).toInt());
    ShellManager::s_forceWindowed = parser.isSet(win);
    ShellManager::s_noRespawn = parser.isSet(respawn);
    ShellManager::s_fixedShell = parser.value(shellPlugin);
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), ShellManager::instance(), SLOT(deleteLater()));

    return app.exec();
}
