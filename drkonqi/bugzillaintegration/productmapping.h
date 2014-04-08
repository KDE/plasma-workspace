/*******************************************************************
* productmapping.h
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

#ifndef PRODUCTMAPPING__H
#define PRODUCTMAPPING__H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

class Product;
class BugzillaManager;
class CrashedApplication;

class ProductMapping: public QObject
{
Q_OBJECT
public:
    explicit ProductMapping(const CrashedApplication *, BugzillaManager *, QObject * parent = 0);

    QString bugzillaProduct() const;
    QString bugzillaComponent() const;
    QString bugzillaVersion() const;
    QStringList relatedBugzillaProducts() const;

    bool bugzillaProductDisabled() const;
    bool bugzillaVersionDisabled() const;

private Q_SLOTS:
    void checkProductInfo(const Product &);

private:
    void map(const QString&);
    void mapUsingInternalFile(const QString&);
    void getRelatedProductsUsingInternalFile(const QString&);

    QStringList m_relatedBugzillaProducts;
    QString     m_bugzillaProduct;
    QString     m_bugzillaComponent;

    QString     m_bugzillaVersionString;

    const CrashedApplication *   m_crashedAppPtr;
    BugzillaManager *   m_bugzillaManagerPtr;

    bool m_bugzillaProductDisabled;
    bool m_bugzillaVersionDisabled;

};

#endif
