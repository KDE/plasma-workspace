/*
 *   Copyright 2010 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "layouttemplatepackagestructure.h"

namespace WorkspaceScripting
{

LayoutTemplatePackageStructure::LayoutTemplatePackageStructure(QObject *parent)
    : Plasma::PackageStructure(parent)
{
    setServicePrefix("plasma-layout-template");
    setDefaultPackageRoot("plasma/layout-templates");
    addFileDefinition("mainscript", "layout.js", i18n("Main Script File"));
    setRequired("mainscript", true);
}

LayoutTemplatePackageStructure::~LayoutTemplatePackageStructure()
{

}

}

#include "layouttemplatepackagestructure.moc"

