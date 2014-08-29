/*
 * Copyright 2012 Marco Martin <mart@kde.org>
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
#include <KLocalizedString>

#include <qcommandlineparser.h>
#include <qcommandlineoption.h>
#include <QAction>
#include <QUrl>
#include <QDebug>
#include <QQuickWindow>
#include <QSessionManager>

#include <KAuthorized>
#include <kdbusservice.h>

#include <kdeclarative/qmlobject.h>

#include "view.h"

static const char version[] = "0.1";
static QCommandLineParser parser;

int main(int argc, char **argv)
{
    KLocalizedString::setApplicationDomain("krunner");

    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);
//     TODO: Make it a QGuiApplication once we don't depend on KDELibs4Support
//     QGuiApplication app(argc, argv);
    app.setApplicationName("krunner");
    app.setOrganizationDomain("kde.org");
    app.setApplicationVersion(version);
    app.setQuitOnLastWindowClosed(false);
    parser.setApplicationDescription(i18n("Run Command interface"));
    KDBusService service(KDBusService::Unique);

    parser.addVersionOption();
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    View view;
    view.setVisible(false);

    QObject::connect(&service, SIGNAL(activateRequested(QStringList,QString)),
                     &view, SLOT(display()));

    return app.exec();
}
