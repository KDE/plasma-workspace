/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "cursorthemesettings.h"

#include "../kcms-common_p.h"

#include "xcursor/cursortheme.h"
#include "xcursor/themeapplicator.h"
#include "xcursor/thememodel.h"

#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTimer>

int main(int argc, char **argv)
{
    // This is a CLI application, but we require at least a QGuiApplication for things
    // in Plasma::Theme, so let's just roll with one of these
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma-apply-cursortheme"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    KLocalizedString::setApplicationDomain("plasma-apply-cursortheme");

    QCommandLineParser *parser = new QCommandLineParser;
    parser->addHelpOption();
    parser->setApplicationDescription(
        i18n("This tool allows you to set the mouse cursor theme for the current Plasma session, without accidentally setting it to one that is either not "
             "available, or which is already set."));
    parser->addPositionalArgument(
        QStringLiteral("cursortheme"),
        i18n("The name of the cursor theme you wish to set for your current Plasma session (passing a full path will only use the last part of the path)"));
    parser->addOption(QCommandLineOption(QStringLiteral("list-themes"), i18n("Show all the themes available on the system (and which is the current theme)")));
    parser->process(app);

    int errorCode{0};
    CursorThemeSettings *settings = new CursorThemeSettings(&app);
    QTextStream ts(stdout);
    CursorThemeModel *model = new CursorThemeModel(&app);
    if (!parser->positionalArguments().isEmpty()) {
        QString requestedTheme{parser->positionalArguments().first()};
        const QString dirSplit{"/"};
        if (requestedTheme.contains(dirSplit)) {
            QStringList splitTheme = requestedTheme.split(dirSplit, Qt::SkipEmptyParts);
            // Cursor themes installed through KNewStuff will commonly be given an installed files entry
            // which has the main directory name and an asterix to say the cursors are all in that directory,
            // and since one of the main purposes of this tool is to allow adopting things from a kns dialog,
            // we handle that little weirdness here.
            splitTheme.removeAll(QStringLiteral("*"));
            requestedTheme = splitTheme.last();
        }

        if (settings->cursorTheme() == requestedTheme) {
            ts << i18n("The requested theme \"%1\" is already set as the theme for the current Plasma session.", requestedTheme) << Qt::endl;
            // This is not an error condition, no reason to set an error code
        } else {
            auto results = model->findIndex(requestedTheme);
            QModelIndex selected = model->index(results.row(), 0);
            const CursorTheme *theme = selected.isValid() ? model->theme(selected) : nullptr;

            if (theme) {
                settings->setCursorTheme(theme->name());
                if (settings->save() && applyTheme(theme, theme->defaultCursorSize())) {
                    notifyKcmChange(GlobalChangeType::CursorChanged);
                    ts << i18n("Successfully applied the mouse cursor theme %1 to your current Plasma session", theme->title()) << Qt::endl;
                } else {
                    ts << i18n("You have to restart the Plasma session for your newly applied mouse cursor theme to display correctly.") << Qt::endl;
                    // A bit of an odd one, more a warning than an error, but this means we can forward it
                    errorCode = -1;
                }
            } else {
                QStringList availableThemes;
                for (int i = 0; i < model->rowCount(); ++i) {
                    const CursorTheme *theme = model->theme(model->index(i, 0));
                    availableThemes << theme->name();
                }
                ts << i18n("Could not find theme \"%1\". The theme should be one of the following options: %2",
                           requestedTheme,
                           availableThemes.join(QLatin1String{", "}))
                   << Qt::endl;
                errorCode = -1;
            }
        }
    } else if (parser->isSet(QStringLiteral("list-themes"))) {
        ts << i18n("You have the following mouse cursor themes on your system:") << Qt::endl;
        for (int i = 0; i < model->rowCount(); ++i) {
            const CursorTheme *theme = model->theme(model->index(i, 0));
            if (settings->cursorTheme() == theme->name()) {
                ts << QString(" * %1 (%2 - current theme for this Plasma session)").arg(theme->title()).arg(theme->name()) << Qt::endl;
            } else {
                ts << QString(" * %1 (%2)").arg(theme->title()).arg(theme->name()) << Qt::endl;
            }
        }
    } else {
        parser->showHelp();
    }
    QTimer::singleShot(0, &app, [&app, &errorCode]() {
        app.exit(errorCode);
    });

    return app.exec();
}
