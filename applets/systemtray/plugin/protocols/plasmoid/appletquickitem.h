/*
 *   Copyright 2014 Marco Martin <mart@kde.org>
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

#ifndef APPLETQUICKITEM_H
#define APPLETQUICKITEM_H

#include <QQuickItem>
#include <QWeakPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTimer>

#include <Plasma/Package>

#include "privateheaders/plasmaquick_export.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public Plasma API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

namespace Plasma {
    class Applet;
}

namespace KDeclarative {
    class QmlObject;
}


namespace PlasmaQuick {

class AppletQuickItemPrivate;

class PLASMAQUICK_EXPORT AppletQuickItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(int switchWidth READ switchWidth WRITE setSwitchWidth NOTIFY switchWidthChanged)
    Q_PROPERTY(int switchHeight READ switchHeight WRITE setSwitchHeight NOTIFY switchHeightChanged)

    Q_PROPERTY(QQmlComponent *compactRepresentation READ compactRepresentation WRITE setCompactRepresentation NOTIFY compactRepresentationChanged)
    Q_PROPERTY(QObject *compactRepresentationItem READ compactRepresentationItem NOTIFY compactRepresentationItemChanged)

    Q_PROPERTY(QQmlComponent *fullRepresentation READ fullRepresentation WRITE setFullRepresentation NOTIFY fullRepresentationChanged)
    Q_PROPERTY(QObject *fullRepresentationItem READ fullRepresentationItem NOTIFY fullRepresentationItemChanged)


    /**
     * this is supposed to be either one between compactRepresentation or fullRepresentation
     */
    Q_PROPERTY(QQmlComponent *preferredRepresentation READ preferredRepresentation WRITE setPreferredRepresentation NOTIFY preferredRepresentationChanged)

    /**
     * True when the applet is showing its full representation. either as the main only view, or in a popup.
     * Setting it will open or close the popup if the plasmoid is iconified, however it won't have effect if the applet is open
     */
    Q_PROPERTY(bool expanded WRITE setExpanded READ isExpanded NOTIFY expandedChanged)

public:
    AppletQuickItem(Plasma::Applet *applet, QQuickItem *parent = 0);
    ~AppletQuickItem();

////API NOT SUPPOSED TO BE USED BY QML
    Plasma::Applet *applet() const;

    //Make the constructor lighter and delay the actual instantiation of the qml in the applet
    virtual void init();

    Plasma::Package appletPackage() const;
    void setAppletPackage(const Plasma::Package &package);

    Plasma::Package coronaPackage() const;
    void setCoronaPackage(const Plasma::Package &package);

    QObject *compactRepresentationItem();
    QObject *fullRepresentationItem();

////PROPERTY ACCESSORS
    int switchWidth() const;
    void setSwitchWidth(int width);

    int switchHeight() const;
    void setSwitchHeight(int width);


    QQmlComponent *compactRepresentation();
    void setCompactRepresentation(QQmlComponent *component);

    QQmlComponent *fullRepresentation();
    void setFullRepresentation(QQmlComponent *component);

    QQmlComponent *preferredRepresentation();
    void setPreferredRepresentation(QQmlComponent *component);

    bool isExpanded() const;
    void setExpanded(bool expanded);

////NEEDED BY QML TO CREATE ATTACHED PROPERTIES
    static AppletQuickItem *qmlAttachedProperties(QObject *object);


Q_SIGNALS:
//Property signals
    void switchWidthChanged(int width);
    void switchHeightChanged(int height);

    void expandedChanged(bool expanded);

    void compactRepresentationChanged(QQmlComponent *compactRepresentation);
    void fullRepresentationChanged(QQmlComponent *fullRepresentation);
    void preferredRepresentationChanged(QQmlComponent *preferredRepresentation);

    void compactRepresentationItemChanged(QObject *compactRepresentationItem);
    void fullRepresentationItemChanged(QObject *fullRepresentationItem);

protected:
    KDeclarative::QmlObject *qmlObject();

    //Reimplementation
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    virtual void itemChange(ItemChange change, const ItemChangeData &value);



private:
    AppletQuickItemPrivate *const d;

    Q_PRIVATE_SLOT(d, void compactRepresentationCheck())
    Q_PRIVATE_SLOT(d, void minimumWidthChanged())
    Q_PRIVATE_SLOT(d, void minimumHeightChanged())
    Q_PRIVATE_SLOT(d, void preferredWidthChanged())
    Q_PRIVATE_SLOT(d, void preferredHeightChanged())
    Q_PRIVATE_SLOT(d, void maximumWidthChanged())
    Q_PRIVATE_SLOT(d, void maximumHeightChanged())
    Q_PRIVATE_SLOT(d, void fillWidthChanged())
    Q_PRIVATE_SLOT(d, void fillHeightChanged())
};

}

QML_DECLARE_TYPEINFO(PlasmaQuick::AppletQuickItem, QML_HAS_ATTACHED_PROPERTIES)


#endif
