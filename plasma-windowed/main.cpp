/*
    SPDX-FileCopyrightText: 2014 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2014 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <csignal>

#include <QApplication>
#include <QQmlDebuggingEnabler>
#include <QSurfaceFormat>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

#include <KDBusService>
#include <KLocalizedString>
#include <KSignalHandler>

#include "plasmawindowedcorona.h"
#include "plasmawindowedview.h"

static const char version[] = "1.0";

int main(int argc, char **argv)
{
#if QT_CONFIG(qml_debug)
    if (qEnvironmentVariableIsSet("PLASMA_ENABLE_QML_DEBUG")) {
        QQmlDebuggingEnabler::enableDebugging(true);
    }
#endif
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QQuickWindow::setDefaultAlphaBuffer(true);

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    // this is needed to fake window position so Plasma Dialog sets correct borders
    qputenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS", {});

    QApplication app(argc, argv);
    qunsetenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS");
    app.setApplicationVersion(QLatin1String(version));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("plasmawindowed"));

    KDBusService service(KDBusService::Unique);

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Plasma Windowed"));
#if QT_CONFIG(qml_debug)
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("d"), QStringLiteral("qmljsdebugger"), i18n("Enable QML Javascript debugger")}));
#endif
    parser.addOption(
        QCommandLineOption(QStringLiteral("statusnotifier"), i18n("Makes the plasmoid stay alive in the Notification Area, even when the window is closed.")));
    QCommandLineOption shellPluginOption(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                         i18n("Force loading the given shell plugin"),
                                         QStringLiteral("plugin"),
                                         QStringLiteral("org.kde.plasma.desktop"));
    parser.addOption(shellPluginOption);
    parser.addPositionalArgument(QStringLiteral("applet"), i18n("The applet to open."));
    parser.addPositionalArgument(QStringLiteral("args"), i18n("Arguments to pass to the plasmoid."), QStringLiteral("[args...]"));
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        parser.showHelp(1);
    }

    auto *corona = new PlasmaWindowedCorona(parser.value(shellPluginOption));

    const QStringList arguments = parser.positionalArguments();
    QVariantList args;
    QStringList::const_iterator constIterator = arguments.constBegin() + 1;
    for (; constIterator != arguments.constEnd(); ++constIterator) {
        args << (*constIterator);
    }
    corona->setHasStatusNotifier(parser.isSet(QStringLiteral("statusnotifier")));
    corona->loadApplet(arguments.first(), args);

    app.connect(&service, &KDBusService::activateRequested, corona, &PlasmaWindowedCorona::activateRequested);

    // Quit on SIGTERM to properly save gcov results
    KSignalHandler::self()->watchSignal(SIGTERM);
    app.connect(KSignalHandler::self(), &KSignalHandler::signalReceived, &app, [&app](int signal) {
        if (signal == SIGTERM) [[likely]] {
            app.quit();
        }
    });

    return app.exec();
}
