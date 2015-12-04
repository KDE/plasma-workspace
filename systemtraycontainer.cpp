/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
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

#include "systemtraycontainer.h"
#include "debug.h"

#include <QDebug>
#include <QQuickItem>

#include <Plasma/Corona>

SystemTrayContainer::SystemTrayContainer(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
}

SystemTrayContainer::~SystemTrayContainer()
{
}

void SystemTrayContainer::init()
{
    Plasma::Containment *cont = containment();
    if (!cont) {
        return;
    }

    Plasma::Corona *c = cont->corona();
    if (!c) {
        return;
    }

    Plasma::Containment *innerCont = c->createContainment("org.kde.plasma.simplesystray");
    if (!innerCont) {
        return;
    }
    setProperty("_plasma_graphicObject", QVariant::fromValue(innerCont->property("_plasma_graphicObject")));
}


K_EXPORT_PLASMA_APPLET_WITH_JSON(systemtraycontainer, SystemTrayContainer, "containermetadata.json")

#include "systemtraycontainer.moc"
