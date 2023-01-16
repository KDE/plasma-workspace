/*
    SPDX-FileCopyrightText: 2014 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2014 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QApplication>
#include <QSurfaceFormat>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

#include <KDBusService>
#include <KLocalizedString>

#include "plasmawindowedcorona.h"
#include "plasmawindowedview.h"

static const char version[] = "1.0";

int main(int argc, char **argv)
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationVersion(QLatin1String(version));
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    KDBusService service(KDBusService::Unique);

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Plasma Windowed"));
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

    PlasmaWindowedCorona *corona = new PlasmaWindowedCorona(parser.value(shellPluginOption));

    const QStringList arguments = parser.positionalArguments();
    QVariantList args;
    QStringList::const_iterator constIterator = arguments.constBegin() + 1;
    for (; constIterator != arguments.constEnd(); ++constIterator) {
        args << (*constIterator);
    }
    corona->setHasStatusNotifier(parser.isSet(QStringLiteral("statusnotifier")));
    corona->loadApplet(arguments.first(), args);

    QObject::connect(&service, &KDBusService::activateRequested, corona, &PlasmaWindowedCorona::activateRequested);

    const int ret = app.exec();
    delete corona;
    return ret;
}
