/***************************************************************************
 *   Copyright 2010 Artur Duque de Souza <asouza@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include <KLocalizedString>
#include <KPackage/Package>

#include "share_package.h"
#include "config-workspace.h"

SharePackage::SharePackage(QObject *, QVariantList)
{
}

void SharePackage::initPackage(KPackage::Package* package)
{
//    KPackage::PackageStructure::initPackage(package);
    package->addDirectoryDefinition("scripts", QStringLiteral("code"), i18n("Executable Scripts"));
    QStringList mimetypes;
    mimetypes << QStringLiteral("text/*");
    package->setMimeTypes( "scripts", mimetypes );

    package->addFileDefinition("mainscript", QStringLiteral("code/main.js"), i18n("Main Script File"));
    package->setDefaultPackageRoot(PLASMA_RELATIVE_DATA_INSTALL_DIR "shareprovider/");
}

K_EXPORT_KPACKAGE_PACKAGE_WITH_JSON(SharePackage, "plasma-packagestructure-share.json")

#include "share_package.moc"

