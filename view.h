/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef VIEW_H
#define VIEW_H

#include <QtQuick/QQuickView>


#include "plasma/corona.h"
#include "plasma/containment.h"

#include "configview.h"

class View : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(int location READ location WRITE setLocation NOTIFY locationChanged)
    Q_PROPERTY(QRectF screenGeometry READ screenGeometry NOTIFY screenGeometryChanged)

public:
    explicit View(Plasma::Corona *corona, QWindow *parent = 0);
    virtual ~View();

    Plasma::Corona *corona() const;

    virtual KConfigGroup config() const;

    void setContainment(Plasma::Containment *cont);
    Plasma::Containment *containment() const;

    //FIXME: Plasma::Types::Location should be something qml can understand
    int location() const;
    void setLocation(int location);

    Plasma::Types::FormFactor formFactor() const;

    QRectF screenGeometry();

protected Q_SLOTS:
    void showConfigurationInterface(Plasma::Applet *applet);

private Q_SLOTS:
    void coronaPackageChanged(const Plasma::Package &package);

Q_SIGNALS:
    void locationChanged(Plasma::Types::Location location);
    void formFactorChanged(Plasma::Types::FormFactor formFactor);
    void containmentChanged();
    void screenGeometryChanged();

private:
    Plasma::Corona *m_corona;
    QWeakPointer<Plasma::Containment> m_containment;
    QWeakPointer<ConfigView> m_configView;
};

#endif // VIEW_H
