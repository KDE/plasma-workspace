/******************************************************************************
*   Copyright 2007-2009 by Aaron Seigo <aseigo@kde.org>                       *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

#include "shellpackage.h"

#include <KLocalizedString>

#include <Plasma/Package>


void ShellPackageStructure::initPackage(Plasma::Package *package)
{
    package->setDefaultPackageRoot("plasma/shells/");

    //Directories
    package->addDirectoryDefinition("applet", "applet", i18n("Applets furniture"));
    package->addDirectoryDefinition("configuration", "configuration", i18n("Applets furniture"));
    package->addDirectoryDefinition("explorer", "explorer", i18n("Explorer UI for adding widgets"));
    package->addDirectoryDefinition("views", "views", i18n("User interface for the views that will show containments"));

    package->setMimeTypes("applet", QStringList() << "text/x-qml");
    package->setMimeTypes("configuration", QStringList() << "text/x-qml");
    package->setMimeTypes("views", QStringList() << "text/x-qml");

    //Files
    //Default layout
    package->addFileDefinition("defaultlayout", "layout.js", i18n("Default layout file"));
    package->addFileDefinition("defaults", "defaults", i18n("Default plugins for containments, containmentActions etc"));
    package->setMimeTypes("defaultlayout", QStringList() << "application/javascript");
    package->setMimeTypes("defaults", QStringList() << "text/plain");

    //Applet furniture
    package->addFileDefinition("appleterror", "applet/AppletError.qml", i18n("Error message shown when an applet fails to load"));
    package->addFileDefinition("compactapplet", "applet/CompactApplet.qml", i18n("QML component that shows an applet in a popup"));
    package->addFileDefinition("defaultcompactrepresentation", "applet/DefaultCompactRepresentation.qml", i18n("Compact representation of an applet when collapsed in a popup, for instance as an icon. applets can override this component."));

    //Configuration
    package->addFileDefinition("appletconfigurationui", "configuration/AppletConfiguration.qml", i18n("QML component for the configuration dialog for applets"));
    package->addFileDefinition("containmentconfigurationui", "configuration/ContainmentConfiguration.qml", i18n("QML component for the configuration dialog for containments"));
    package->addFileDefinition("panelconfigurationui", "configuration/PanelConfiguration.qml", i18n("Panel configuration UI"));

    //WIdget explorer
    package->addFileDefinition("widgetexplorer", "explorer/WidgetExplorer.qml", i18n("Widgets explorer UI"));
    
    
    //package->setRequired("mainscript", true);
}


