/*******************************************************************
* productmapping.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#include "productmapping.h"

#include <KConfig>
#include <KConfigGroup>
#include <QDebug>
#include <QStandardPaths>

#include "bugzillalib.h"
#include "crashedapplication.h"

ProductMapping::ProductMapping(const CrashedApplication * crashedApp, BugzillaManager * bzManager, QObject * parent)
    : QObject(parent)
    , m_crashedAppPtr(crashedApp)
    , m_bugzillaManagerPtr(bzManager)
    , m_bugzillaProductDisabled(false)
    , m_bugzillaVersionDisabled(false)

{
    //Default "fallback" values
    m_bugzillaProduct = crashedApp->fakeExecutableBaseName();
    m_bugzillaComponent = QLatin1String("general");
    m_bugzillaVersionString = QLatin1String("unspecified");
    m_relatedBugzillaProducts = QStringList() << m_bugzillaProduct;

    map(crashedApp->fakeExecutableBaseName());

    //Get valid versions
    connect(m_bugzillaManagerPtr, &BugzillaManager::productInfoFetched, this, &ProductMapping::checkProductInfo);

    m_bugzillaManagerPtr->fetchProductInfo(m_bugzillaProduct);
}

void ProductMapping::map(const QString & appName)
{
    mapUsingInternalFile(appName);
    getRelatedProductsUsingInternalFile(m_bugzillaProduct);
}

void ProductMapping::mapUsingInternalFile(const QString & appName)
{
    KConfig mappingsFile(QString::fromLatin1("mappings"), KConfig::NoGlobals, QStandardPaths::DataLocation);
    const KConfigGroup mappings = mappingsFile.group("Mappings");
    if (mappings.hasKey(appName)) {
        QString mappingString = mappings.readEntry(appName);
        if (!mappingString.isEmpty()) {
            QStringList list = mappingString.split('|', QString::SkipEmptyParts);
            if (list.count()==2) {
                m_bugzillaProduct = list.at(0);
                m_bugzillaComponent = list.at(1);
                m_relatedBugzillaProducts = QStringList() << m_bugzillaProduct;
            } else {
                qWarning() << "Error while reading mapping entry. Sections found " << list.count();
            }
        } else {
            qWarning() << "Error while reading mapping entry. Entry exists but it is empty "
                            "(or there was an error when reading)";
        }
    }
}

void ProductMapping::getRelatedProductsUsingInternalFile(const QString & bugzillaProduct)
{
    //ProductGroup ->  kontact=kdepim
    //Groups -> kdepim=kontact|kmail|korganizer|akonadi|pimlibs..etc

    KConfig mappingsFile(QString::fromLatin1("mappings"), KConfig::NoGlobals, QStandardPaths::DataLocation);
    const KConfigGroup productGroup = mappingsFile.group("ProductGroup");

    //Get groups of the application
    QStringList groups;
    if (productGroup.hasKey(bugzillaProduct)) {
        QString group = productGroup.readEntry(bugzillaProduct);
        if (group.isEmpty()) {
            qWarning() << "Error while reading mapping entry. Entry exists but it is empty "
                            "(or there was an error when reading)";
            return;
        }
        groups = group.split('|', QString::SkipEmptyParts);
    }

    //All KDE apps use the KDE Platform (basic libs)
    groups << QLatin1String("kdeplatform");

    //Add the product itself
    m_relatedBugzillaProducts = QStringList() << m_bugzillaProduct;

    //Get related products of each related group
    Q_FOREACH( const QString & group, groups ) {
        const KConfigGroup bzGroups = mappingsFile.group("BZGroups");
        if (bzGroups.hasKey(group)) {
            QString bzGroup = bzGroups.readEntry(group);
            if (!bzGroup.isEmpty()) {
                QStringList relatedGroups = bzGroup.split('|', QString::SkipEmptyParts);
                if (relatedGroups.size()>0) {
                    m_relatedBugzillaProducts.append(relatedGroups);
                }
            } else {
                qWarning() << "Error while reading mapping entry. Entry exists but it is empty "
                                "(or there was an error when reading)";
            }
        }
    }
}

void ProductMapping::checkProductInfo(const Product & product)
{
    // check whether the product itself is disabled for new reports,
    // which usually means that product/application is unmaintained.
    m_bugzillaProductDisabled = !product.isActive();

    // check whether the product on bugzilla contains the expected component
    if (! product.components().contains(m_bugzillaComponent)) {
        m_bugzillaComponent = QLatin1String("general");
    }

    // find the appropriate version to use on bugzilla
    const QString version = m_crashedAppPtr->version();
    const QStringList& allVersions = product.allVersions();

    if (allVersions.contains(version)) {
        //The version the crash application provided is a valid bugzilla version: use it !
        m_bugzillaVersionString = version;
    } else if (version.endsWith(QLatin1String(".00"))) {
        //check if there is a version on bugzilla with just ".0"
        const QString shorterVersion = version.left(version.size() - 1);
        if (allVersions.contains(shorterVersion)) {
            m_bugzillaVersionString = shorterVersion;
        }
    }

    // check whether that verions is disabled for new reports, which
    // usually means that version is outdated and not supported anymore.
    const QStringList& inactiveVersions = product.inactiveVersions();
    m_bugzillaVersionDisabled = inactiveVersions.contains(m_bugzillaVersionString);

}

QStringList ProductMapping::relatedBugzillaProducts() const
{
    return m_relatedBugzillaProducts;
}

QString ProductMapping::bugzillaProduct() const
{
    return m_bugzillaProduct;
}

QString ProductMapping::bugzillaComponent() const
{
    return m_bugzillaComponent;
}

QString ProductMapping::bugzillaVersion() const
{
    return m_bugzillaVersionString;
}

bool ProductMapping::bugzillaProductDisabled() const
{
    return m_bugzillaProductDisabled;
}

bool ProductMapping::bugzillaVersionDisabled() const
{
    return m_bugzillaVersionDisabled;
}
