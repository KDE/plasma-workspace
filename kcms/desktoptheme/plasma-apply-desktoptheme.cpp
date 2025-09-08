/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "themesmodel.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QTimer>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    // This is a CLI application, but we require at least a QGuiApplication for things
    // in Plasma::Theme, so let's just roll with one of these
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma-apply-desktoptheme"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("plasma-apply-desktoptheme"));

    auto *parser = new QCommandLineParser;
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

    KConfig plasmarc(u"plasmarc"_s);
    KConfigGroup themeGroup = plasmarc.group(u"Theme"_s);

    if (!parser->positionalArguments().isEmpty()) {
        QString requestedTheme{parser->positionalArguments().constFirst()};
        constexpr QLatin1Char dirSplit{'/'};
        if (requestedTheme.contains(dirSplit)) {
            requestedTheme = requestedTheme.split(dirSplit, Qt::SkipEmptyParts).last();
        }
        if (themeGroup.readEntry("name", u"default"_s) == requestedTheme) {
            ts << i18n("The requested theme \"%1\" is already set as the theme for the current Plasma session.", requestedTheme) << Qt::endl;
            // Not an error condition really, let's just ignore that
        } else {
            bool found{false};
            QStringList availableThemes;
            model->load();
            for (int i = 0; i < model->rowCount(); ++i) {
                QString currentTheme{model->data(model->index(i), ThemesModel::PluginNameRole).toString()};
                if (currentTheme == requestedTheme) {
                    if (requestedTheme == u"default" && !themeGroup.hasDefault(u"name"_s)) {
                        themeGroup.revertToDefault("name", KConfig::Notify);
                    } else {
                        themeGroup.writeEntry("name", requestedTheme, KConfig::Notify);
                    }

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
            if (themeGroup.readEntry("name", u"default"_s) == themeName) {
                ts << u" * %1 (current theme for this Plasma session)"_s.arg(themeName) << Qt::endl;
            } else {
                ts << u" * %1"_s.arg(themeName) << Qt::endl;
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
