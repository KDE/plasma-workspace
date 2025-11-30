/*
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klookandfeelmanager.h"
#include "lookandfeelsettings.h"

#include <iostream>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

// Frameworks
#include <KAboutData>
#include <KLocalizedString>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // About data
    KAboutData aboutData(u"plasma-apply-lookandfeel"_s,
                         i18n("Global Theme Tool"),
                         u"1.0"_s,
                         i18n("Command line tool to apply global theme packages for changing the look and feel."),
                         KAboutLicense::GPL,
                         i18n("Copyright 2017, Marco Martin"));
    aboutData.addAuthor(i18n("Marco Martin"), i18n("Maintainer"), QStringLiteral("mart@kde.org"));
    aboutData.setDesktopFileName(u"org.kde.plasma-apply-lookandfeel"_s);
    KAboutData::setApplicationData(aboutData);

    const static auto _l = QStringLiteral("list");
    const static auto _a = QStringLiteral("apply");
    const static auto _r = QStringLiteral("resetLayout");

    QCommandLineOption _list = QCommandLineOption(QStringList() << QStringLiteral("l") << _l, i18n("List available global theme packages"));
    QCommandLineOption _apply =
        QCommandLineOption(QStringList() << QStringLiteral("a") << _a,
                           i18n("Apply a global theme package. This can be the name of a package, or a full path to an installed package, at which point this "
                                "tool will ensure it is a global theme package and then attempt to apply it"),
                           i18n("packagename"));
    QCommandLineOption _resetLayout = QCommandLineOption(QStringList() << _r, i18n("Reset the Plasma Desktop layout"));

    QCommandLineParser parser;
    parser.addOption(_list);
    parser.addOption(_apply);
    parser.addOption(_resetLayout);
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!parser.isSet(_list) && !parser.isSet(_apply)) {
        parser.showHelp();
    }

    if (parser.isSet(_list)) {
        const QList<KPluginMetaData> pkgs = KPackage::PackageLoader::self()->listPackages(u"Plasma/LookAndFeel"_s);

        for (const KPluginMetaData &data : pkgs) {
            std::cout << data.pluginId().toStdString() << std::endl;
        }

    } else if (parser.isSet(_apply)) {
        QString requestedTheme{parser.value(_apply)};
        QFileInfo info(requestedTheme);
        // Check if the theme name passed validates as the absolute path for a folder
        if (info.isDir()) {
            // absolute paths need to be passed with trailing shash to KPackage
            requestedTheme += QStringLiteral("/");
        }
        KPackage::Package p = KPackage::PackageLoader::self()->loadPackage(u"Plasma/LookAndFeel"_s);
        p.setPath(requestedTheme);

        // can't use package.isValid as lnf packages always fallback, even when not existing
        if (p.metadata().pluginId() != requestedTheme) {
            if (!p.path().isEmpty() && p.path() == requestedTheme && QFile(p.path()).exists()) {
                std::cout << "Absolute path to theme passed in, set that" << std::endl;
                requestedTheme = p.metadata().pluginId();
            } else {
                std::cout << "Unable to find the theme named " << requestedTheme.toStdString() << std::endl;
                return 1;
            }
        }

        // By default do not modify the layout, unless explicitly specified
        KLookAndFeelManager::Contents selection = KLookAndFeelManager::AppearanceSettings;
        if (parser.isSet(_resetLayout)) {
            selection |= KLookAndFeelManager::LayoutSettings;
        }

        LookAndFeelSettings settings;
        settings.setAutomaticLookAndFeel(false);
        settings.setLookAndFeelPackage(requestedTheme);
        settings.save();

        KLookAndFeelManager manager;
        manager.save(p, selection);
    }

    return 0;
}
