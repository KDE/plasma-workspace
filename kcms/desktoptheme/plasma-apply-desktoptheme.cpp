/*
   Copyright (c) 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "themesmodel.h"

#include <Plasma/Theme>

#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QTimer>

int main(int argc, char **argv)
{
    // This is a CLI application, but we require at least a QGuiApplication for things
    // in Plasma::Theme, so let's just roll with one of these
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma-apply-desktoptheme"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    KLocalizedString::setApplicationDomain("plasma-apply-desktoptheme");

    QCommandLineParser *parser = new QCommandLineParser;
    parser->addHelpOption();
    parser->setApplicationDescription(
        i18n("This tool allows you to set the theme of the current Plasma session, without accidentally setting it to one that is either not available, or "
             "which is already set."));
    parser->addPositionalArgument(
        QStringLiteral("themename"),
        i18n("The name of the theme you wish to set for your current Plasma session (passing a full path will only use the last part of the path)"));
    parser->addOption(QCommandLineOption(QStringLiteral("list-themes"), i18n("Show all the themes available on the system (and which is the current theme)")));
    parser->process(app);

    int errorCode{0};
    QTextStream ts(stdout);
    ThemesModel *model{new ThemesModel(&app)};
    if (!parser->positionalArguments().isEmpty()) {
        QString requestedTheme{parser->positionalArguments().first()};
        const QString dirSplit{"/"};
        if (requestedTheme.contains(dirSplit)) {
            requestedTheme = requestedTheme.split(dirSplit, Qt::SkipEmptyParts).last();
        }
        if (Plasma::Theme().themeName() == requestedTheme) {
            ts << i18n("The requested theme \"%1\" is already set as the theme for the current Plasma session.", requestedTheme) << Qt::endl;
            // Not an error condition really, let's just ignore that
        } else {
            bool found{false};
            QStringList availableThemes;
            model->load();
            for (int i = 0; i < model->rowCount(); ++i) {
                QString currentTheme{model->data(model->index(i), ThemesModel::PluginNameRole).toString()};
                if (currentTheme == requestedTheme) {
                    Plasma::Theme().setThemeName(requestedTheme);
                    found = true;
                    break;
                }
                availableThemes << currentTheme;
            }
            if (found) {
                ts << i18n("The current Plasma session's theme has been set to %1", requestedTheme) << Qt::endl;
            } else {
                ts << i18n("Could not find theme \"%1\". The theme should be one of the following options: %2",
                           requestedTheme,
                           availableThemes.join(QLatin1String{", "}))
                   << Qt::endl;
                errorCode = -1;
            }
        }
    } else if (parser->isSet(QStringLiteral("list-themes"))) {
        ts << i18n("You have the following Plasma themes on your system:") << Qt::endl;
        model->load();
        for (int i = 0; i < model->rowCount(); ++i) {
            QString themeName{model->data(model->index(i), ThemesModel::PluginNameRole).toString()};
            if (Plasma::Theme().themeName() == themeName) {
                ts << QString(" * %1 (current theme for this Plasma session)").arg(themeName) << Qt::endl;
            } else {
                ts << QString(" * %1").arg(themeName) << Qt::endl;
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
