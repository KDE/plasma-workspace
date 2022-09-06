/*
    SPDX-FileCopyrightText: 2013 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "shellpackage.h"
#include <KLocalizedString>
#include <KPackage/PackageLoader>

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#define DEFAULT_SHELL "org.kde.plasma.desktop"

ShellPackage::ShellPackage(QObject *, const QVariantList &)
{
}

void ShellPackage::initPackage(KPackage::Package *package)
{
    package->setDefaultPackageRoot(QStringLiteral("plasma/shells/"));

    // Directories
    package->addDirectoryDefinition("applet", QStringLiteral("applet"), i18n("Applets furniture"));
    package->addDirectoryDefinition("configuration", QStringLiteral("configuration"), i18n("Applets furniture"));
    package->addDirectoryDefinition("explorer", QStringLiteral("explorer"), i18n("Explorer UI for adding widgets"));
    package->addDirectoryDefinition("views", QStringLiteral("views"), i18n("User interface for the views that will show containments"));

    package->setMimeTypes("applet", QStringList() << QStringLiteral("text/x-qml"));
    package->setMimeTypes("configuration", QStringList() << QStringLiteral("text/x-qml"));
    package->setMimeTypes("views", QStringList() << QStringLiteral("text/x-qml"));

    // Files
    // Default layout
    package->addFileDefinition("defaultlayout", QStringLiteral("layout.js"), i18n("Default layout file"));
    package->addFileDefinition("defaults", QStringLiteral("defaults"), i18n("Default plugins for containments, containmentActions, etc."));
    package->setMimeTypes("defaultlayout", QStringList() << QStringLiteral("application/javascript") << QStringLiteral("text/javascript"));
    package->setMimeTypes("defaults", QStringList() << QStringLiteral("text/plain"));

    // Applet furniture
    package->addFileDefinition("appleterror", QStringLiteral("applet/AppletError.qml"), i18n("Error message shown when an applet fails to load"));
    package->addFileDefinition("compactapplet", QStringLiteral("applet/CompactApplet.qml"), i18n("QML component that shows an applet in a popup"));
    package->addFileDefinition(
        "defaultcompactrepresentation",
        QStringLiteral("applet/DefaultCompactRepresentation.qml"),
        i18n("Compact representation of an applet when collapsed in a popup, for instance as an icon. Applets can override this component."));

    // Configuration
    package->addFileDefinition("appletconfigurationui",
                               QStringLiteral("configuration/AppletConfiguration.qml"),
                               i18n("QML component for the configuration dialog for applets"));
    package->addFileDefinition("containmentconfigurationui",
                               QStringLiteral("configuration/ContainmentConfiguration.qml"),
                               i18n("QML component for the configuration dialog for containments"));
    package->addFileDefinition("panelconfigurationui", QStringLiteral("configuration/PanelConfiguration.qml"), i18n("Panel configuration UI"));
    package->addFileDefinition("appletalternativesui",
                               QStringLiteral("explorer/AppletAlternatives.qml"),
                               i18n("QML component for choosing an alternate applet"));
    package->addFileDefinition("containmentmanagementui",
                               QStringLiteral("configuration/ShellContainmentConfiguration.qml"),
                               i18n("QML component for the configuration dialog of containments"));

    // Widget explorer
    package->addFileDefinition("widgetexplorer", QStringLiteral("explorer/WidgetExplorer.qml"), i18n("Widgets explorer UI"));

    package->addFileDefinition("interactiveconsole",
                               QStringLiteral("InteractiveConsole.qml"),
                               i18n("A UI for writing, loading and running desktop scripts in the current live session"));
}

void ShellPackage::pathChanged(KPackage::Package *package)
{
    if (!package->metadata().isValid()) {
        return;
    }

    const QString pluginName = package->metadata().pluginId();
    if (!pluginName.isEmpty() && pluginName != DEFAULT_SHELL) {
        const QString fallback = package->metadata().value("X-Plasma-FallbackPackage", QStringLiteral(DEFAULT_SHELL));

        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Shell"), fallback);
        package->setFallbackPackage(pkg);
    } else if (package->fallbackPackage().isValid() && pluginName == DEFAULT_SHELL) {
        package->setFallbackPackage(KPackage::Package());
    }
}

K_PLUGIN_CLASS_WITH_JSON(ShellPackage, "plasma-packagestructure-plasma-shell.json")

#include "shellpackage.moc"
