/*
    SPDX-FileCopyrightText: 2013 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "shellpackage.h"
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
    package->addDirectoryDefinition("applet", QStringLiteral("applet"));
    package->addDirectoryDefinition("configuration", QStringLiteral("configuration"));
    package->addDirectoryDefinition("explorer", QStringLiteral("explorer"));
    package->addDirectoryDefinition("views", QStringLiteral("views"));

    package->setMimeTypes("applet", QStringList() << QStringLiteral("text/x-qml"));
    package->setMimeTypes("configuration", QStringList() << QStringLiteral("text/x-qml"));
    package->setMimeTypes("views", QStringList() << QStringLiteral("text/x-qml"));

    // Files
    // Default layout
    package->addFileDefinition("defaultlayout", QStringLiteral("layout.js"));
    package->addFileDefinition("defaults", QStringLiteral("defaults"));
    package->setMimeTypes("defaultlayout", QStringList() << QStringLiteral("application/javascript") << QStringLiteral("text/javascript"));
    package->setMimeTypes("defaults", QStringList() << QStringLiteral("text/plain"));

    // Applet furniture
    package->addFileDefinition("appleterror", QStringLiteral("applet/AppletError.qml"));
    package->addFileDefinition("compactapplet", QStringLiteral("applet/CompactApplet.qml"));
    package->addFileDefinition("defaultcompactrepresentation", QStringLiteral("applet/DefaultCompactRepresentation.qml"));

    // Configuration
    package->addFileDefinition("appletconfigurationui", QStringLiteral("configuration/AppletConfiguration.qml"));
    package->addFileDefinition("containmentconfigurationui", QStringLiteral("configuration/ContainmentConfiguration.qml"));
    package->addFileDefinition("panelconfigurationui", QStringLiteral("configuration/PanelConfiguration.qml"));
    package->addFileDefinition("appletalternativesui", QStringLiteral("explorer/AppletAlternatives.qml"));
    package->addFileDefinition("containmentmanagementui", QStringLiteral("configuration/ShellContainmentConfiguration.qml"));

    // Widget explorer
    package->addFileDefinition("widgetexplorer", QStringLiteral("explorer/WidgetExplorer.qml"));

    package->addFileDefinition("interactiveconsole", QStringLiteral("InteractiveConsole.qml"));
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
