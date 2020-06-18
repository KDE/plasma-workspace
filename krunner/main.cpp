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
#include <QDBusMessage>
#include <QDBusConnection>

#include <KAuthorized>
#include <KAboutData>
#include <KDBusService>

#include <kdeclarative/qmlobject.h>
#include <KQuickAddons/QtQuickSettings>

#include <kworkspace.h>

#include "view.h"

int main(int argc, char **argv)
{
    QCommandLineParser parser;
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    if (!qEnvironmentVariableIsSet("PLASMA_USE_QT_SCALING")) {
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    }

    const bool qpaVariable = qEnvironmentVariableIsSet("QT_QPA_PLATFORM");
    KWorkSpace::detectPlatform(argc, argv);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);
    if (!qpaVariable) {
        // don't leak the env variable to processes we start
        qunsetenv("QT_QPA_PLATFORM");
    }
    KLocalizedString::setApplicationDomain("krunner");

    KQuickAddons::QtQuickSettings::init();

//     TODO: Make it a QGuiApplication once we don't depend on KDELibs4Support
//     QGuiApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("krunner"),
        i18n("krunner"),
        QStringLiteral(PROJECT_VERSION),
        i18n("Run Command interface"),
        KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);
    app.setQuitOnLastWindowClosed(false);

    QCommandLineOption clipboardOption({QStringLiteral("c"), QStringLiteral("clipboard")},
                                        i18n("Use the clipboard contents as query for KRunner"));
    QCommandLineOption daemonOption({QStringLiteral("d"), QStringLiteral("daemon")},
                                        i18n("Start KRunner in the background, don't show it."));
    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));

    parser.addOption(clipboardOption);
    parser.addOption(daemonOption);
    parser.addOption(replaceOption);
    parser.addPositionalArgument(QStringLiteral("query"), i18n("The query to run, only used if -c is not provided"));

    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!KAuthorized::authorize(QStringLiteral("run_command"))) {
        return -1;
    }

    KDBusService service(KDBusService::Unique | KDBusService::StartupOption(parser.isSet(replaceOption) ? KDBusService::Replace : 0));

    View view;

    auto updateVisibility = [&]() {
        const QString query = parser.positionalArguments().value(0);

        if (parser.isSet(daemonOption)) {
            view.setVisible(false);
        } else if (parser.isSet(clipboardOption)) {
            view.displayWithClipboardContents();
        } else if (!query.isEmpty()) {
            view.query(query);
        } else {
            view.display();
        }
    };

    updateVisibility();

    QObject::connect(&service, &KDBusService::activateRequested, &view, [&](const QStringList &arguments, const QString &workingDirectory) {
        Q_UNUSED(workingDirectory)
        parser.parse(arguments);
        updateVisibility();
    });

    return app.exec();
}
