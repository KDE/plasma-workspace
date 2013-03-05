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
    package->addDirectoryDefinition("components", "components", i18n("UI components"));
    package->addDirectoryDefinition("views", "views", i18n("User interface for the views that will show containments"));

    package->setMimeTypes("components", QStringList() << "text/x-qml");
    package->setMimeTypes("views", QStringList() << "text/x-qml");

    //Files
    package->addFileDefinition("layout", "layout.js", i18n("Default layout file"));
    package->addFileDefinition("defaults", "defaults", i18n("Default plugins for containments, containmentActions etc"));

    package->setMimeTypes("layout", QStringList() << "application/javascript");
    package->setMimeTypes("defaults", QStringList() << "text/plain");

    package->addFileDefinition("appleterror", "components/AppletError.qml", i18n("Error message shown when an applet fails loading"));
    package->addFileDefinition("compactapplet", "components/CompactApplet.qml", i18n("QML component that shows an applet in a popup"));
    package->addFileDefinition("configurationui", "components/Configuration.qml", i18n("QML component for the configuratuion dialog"));
    package->addFileDefinition("defaultcompactrepresentation", "components/DefaultCompactRepresentation.qml", i18n("Compact representation of an applet when collapsed in a popup, for instance as an icon. applets can override this component."));
    package->addFileDefinition("widgetexplorer", "components/WidgetExplorer.qml", i18n("Widgets explorer UI"));
    
    //package->setRequired("mainscript", true);
}


