/******************************************************************************
*   Copyright 2007-2009 by Aaron Seigo <aseigo@kde.org>                       *
*   Copyright 2019 by Marco Martin <mart@kde.org>                             *
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

#include <KLocalizedString>
#include <kpackage/package.h>
#include <kpackage/packagestructure.h>

class SensorAppletPackage : public KPackage::PackageStructure
{
    Q_OBJECT
public:
    SensorAppletPackage(QObject *parent = nullptr, const QVariantList &args = QVariantList()) : KPackage::PackageStructure(parent, args) {}

    void initPackage(KPackage::Package *package) override
    {
        package->setDefaultPackageRoot(QStringLiteral("ksysguard/sensorapplet"));

        package->addDirectoryDefinition("ui", QStringLiteral("ui"), i18n("User Interface"));

        package->addFileDefinition("CompactRepresentation", QStringLiteral("ui/CompactRepresentation.qml"), i18n("The compact representation of the sensors plasmoid when collapsed, for instance in a panel."));
        package->setRequired("CompactRepresentation", true);

        package->addFileDefinition("FullRepresentation", QStringLiteral("ui/FullRepresentation.qml"), i18n("The representation of the plasmoid when it's fully expanded."));
        package->setRequired("FullRepresentation", true);

        package->addFileDefinition("ConfigUI", QStringLiteral("ui/Config.qml"), i18n("The optional configuration page for this face."));

        package->addDirectoryDefinition("config", QStringLiteral("config"), i18n("Configuration support"));
        package->addFileDefinition("mainconfigxml", QStringLiteral("config/main.xml"), i18n("KConfigXT xml file for face-specific configuration options."));
    }
};

K_EXPORT_KPACKAGE_PACKAGE_WITH_JSON(SensorAppletPackage, "sensorapplet-packagestructure.json")

#include "sensorappletpackage.moc"
