/*
    SPDX-FileCopyrightText: 2007-2009 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KPackage/PackageLoader>
#include <KPackage/PackageStructure>

#define DEFAULT_LOOKANDFEEL "org.kde.breeze.desktop"

class LookAndFeelPackage : public KPackage::PackageStructure
{
    Q_OBJECT
public:
    using KPackage::PackageStructure::PackageStructure;

    void initPackage(KPackage::Package *package)
    {
        // https://community.kde.org/Plasma/lookAndFeelPackage#
        package->setDefaultPackageRoot(QStringLiteral("plasma/look-and-feel/"));

        // Defaults
        package->removeDefinition("mainscript"); // This doesn't make sense, because we don't have one single entrypoint
        package->addFileDefinition("defaults", QStringLiteral("defaults"));
        package->addFileDefinition("layoutdefaults", QStringLiteral("layouts/defaults"));
        package->addDirectoryDefinition("plasmoidsetupscripts", QStringLiteral("plasmoidsetupscripts"));
        // Colors
        package->addFileDefinition("colors", QStringLiteral("colors"));

        // Directories
        package->addDirectoryDefinition("previews", QStringLiteral("previews"));
        package->addFileDefinition("preview", QStringLiteral("previews/preview.png"));
        package->addFileDefinition("fullscreenpreview", QStringLiteral("previews/fullscreenpreview.jpg"));
        package->addFileDefinition("loginmanagerpreview", QStringLiteral("previews/loginmanager.png"));
        package->addFileDefinition("lockscreenpreview", QStringLiteral("previews/lockscreen.png"));
        package->addFileDefinition("userswitcherpreview", QStringLiteral("previews/userswitcher.png"));
        package->addFileDefinition("splashpreview", QStringLiteral("previews/splash.png"));
        package->addFileDefinition("runcommandpreview", QStringLiteral("previews/runcommand.png"));
        package->addFileDefinition("windowdecorationpreview", QStringLiteral("previews/windowdecoration.png"));
        package->addFileDefinition("windowswitcherpreview", QStringLiteral("previews/windowswitcher.png"));

        package->addDirectoryDefinition("loginmanager", QStringLiteral("loginmanager"));
        package->addFileDefinition("loginmanagermainscript", QStringLiteral("loginmanager/LoginManager.qml"));

        package->addDirectoryDefinition("logout", QStringLiteral("logout"));
        package->addFileDefinition("logoutmainscript", QStringLiteral("logout/Logout.qml"));

        package->addDirectoryDefinition("lockscreen", QStringLiteral("lockscreen"));
        package->addFileDefinition("lockscreenmainscript", QStringLiteral("lockscreen/LockScreen.qml"));

        package->addDirectoryDefinition("userswitcher", QStringLiteral("userswitcher"));
        package->addFileDefinition("userswitchermainscript", QStringLiteral("userswitcher/UserSwitcher.qml"));

        package->addDirectoryDefinition("osd", QStringLiteral("osd"));
        package->addFileDefinition("osdmainscript", QStringLiteral("osd/Osd.qml"));

        package->addDirectoryDefinition("splash", QStringLiteral("splash"));
        package->addFileDefinition("splashmainscript", QStringLiteral("splash/Splash.qml"));

        package->addDirectoryDefinition("runcommand", QStringLiteral("runcommand"));
        package->addFileDefinition("runcommandmainscript", QStringLiteral("runcommand/RunCommand.qml"));

        package->addDirectoryDefinition("windowdecoration", QStringLiteral("windowdecoration"));
        package->addFileDefinition("windowdecorationmainscript", QStringLiteral("windowdecoration/WindowDecoration.qml"));

        package->addDirectoryDefinition("windowswitcher", QStringLiteral("windowswitcher"));
        package->addFileDefinition("windowswitchermainscript", QStringLiteral("windowswitcher/WindowSwitcher.qml"));

        package->addDirectoryDefinition("systemdialog", QStringLiteral("systemdialog"));
        package->addFileDefinition("systemdialogscript", QStringLiteral("systemdialog/SystemDialog.qml"));

        package->addDirectoryDefinition("layouts", QStringLiteral("layouts"));

        package->setPath(DEFAULT_LOOKANDFEEL);
    }

    void pathChanged(KPackage::Package *package)
    {
        if (!package->metadata().isValid()) {
            return;
        }

        const QString pluginName = package->metadata().pluginId();

        if (!pluginName.isEmpty() && pluginName != DEFAULT_LOOKANDFEEL) {
            KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), DEFAULT_LOOKANDFEEL);
            package->setFallbackPackage(pkg);
        } else if (package->fallbackPackage().isValid() && pluginName == DEFAULT_LOOKANDFEEL) {
            package->setFallbackPackage(KPackage::Package());
        }
    }
};

K_PLUGIN_CLASS_WITH_JSON(LookAndFeelPackage, "plasma-packagestructure-lookandfeel.json")

#include "lookandfeel.moc"
