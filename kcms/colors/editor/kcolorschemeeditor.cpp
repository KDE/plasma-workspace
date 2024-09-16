/* KDE Display scheme editor
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "scmeditordialog.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTextStream>

#include <KAboutData>
#include <KWindowSystem>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("kcolorschemeeditor"),
                         i18n("KColorSchemeEditor"),
                         QStringLiteral("0.1"),
                         i18n("Utility to edit and create color schemes"),
                         KAboutLicense::GPL_V3);
    aboutData.addAuthor(i18n("Olivier Churlaud"), i18n("Utility creation"), QStringLiteral("olivier@churlaud.com"));
    aboutData.addAuthor(i18n("Jeremy Whiting"), i18n("KCM code (reused in here)"), QStringLiteral("jpwhiting@kde.org"));
    aboutData.addAuthor(i18n("Matthew Woehlke"), i18n("KCM code (reused in here)"), QStringLiteral("mw_triad@users.sourceforge.net"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addPositionalArgument(u"theme"_s, i18n("Scheme to edit or to use as a base."), QStringLiteral("kcolorschemeeditor ThemeName"));

    QCommandLineOption overwriteOption(QStringLiteral("overwrite"), i18n("Show 'Apply' button that saves changes without asking (unlike 'Save As' button)"));
    parser.addOption(overwriteOption);

    QCommandLineOption attachOption(QStringLiteral("attach"),
                                    i18n("Makes the dialog transient for another application window specified by handle"),
                                    QStringLiteral("handle"));
    parser.addOption(attachOption);

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    const QStringList args = parser.positionalArguments();
    QString path;
    if (args.count() == 1) {
        const QString fileBaseName(args.at(0));
        path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"color-schemes/" + fileBaseName + u".colors");
    }
    if (path.isEmpty()) {
        QTextStream out(stderr);
        out << i18n("Scheme not found, falling back to current one.\n");
    }

    SchemeEditorDialog dialog(path);
    dialog.setShowApplyOverwriteButton(parser.isSet(overwriteOption));

    // FIXME doesn't work :(
    const QString attachHandle = parser.value(attachOption);
    if (!attachHandle.isEmpty()) {
        // TODO wayland: once we have foreign surface support
        const QString x11Prefix = QStringLiteral("x11:");

        if (attachHandle.startsWith(x11Prefix)) {
            bool ok = false;
            WId winId = attachHandle.mid(x11Prefix.length()).toLong(&ok, 0);
            if (ok) {
                dialog.setModal(true);
                dialog.setAttribute(Qt::WA_NativeWindow, true);
                KWindowSystem::setMainWindow(dialog.windowHandle(), winId);
                QObject::connect(&dialog, &QDialog::finished, &app, &QApplication::quit);
            }
        }
    }

    dialog.show();

    return app.exec();
}
