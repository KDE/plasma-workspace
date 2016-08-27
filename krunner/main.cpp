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

#include <QCommandLineParser>
#include <QAction>
#include <QUrl>
#include <QDebug>
#include <QQuickWindow>
#include <QSessionManager>

#include <KAuthorized>
#include <KAboutData>
#include <KDBusService>

#include <kdeclarative/qmlobject.h>
#include <KQuickAddons/QtQuickSettings>


#include "view.h"

static QCommandLineParser parser;

int main(int argc, char **argv)
{
    qunsetenv("QT_DEVICE_PIXEL_RATIO");
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    KLocalizedString::setApplicationDomain("krunner");

    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);

    KQuickAddons::QtQuickSettings::init();

//     TODO: Make it a QGuiApplication once we don't depend on KDELibs4Support
//     QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("krunner"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setApplicationVersion(QStringLiteral(PROJECT_VERSION));
    app.setQuitOnLastWindowClosed(false);
    parser.setApplicationDescription(i18n("Run Command interface"));

    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    if (!KAuthorized::authorize(QStringLiteral("run_command"))) {
        return -1;
    }

    KDBusService service(KDBusService::Unique);

    KAboutData aboutData(QStringLiteral("krunner"),
        i18n("krunner"),
        QStringLiteral(PROJECT_VERSION),
        i18n("Run Command interface"),
        KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);

    QGuiApplication::setFallbackSessionManagementEnabled(false);

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    View view;
    view.setVisible(false);

    QObject::connect(&service, &KDBusService::activateRequested, &view, &View::display);

    return app.exec();
}
