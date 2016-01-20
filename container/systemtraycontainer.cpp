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

#include <QDebug>
#include <QQuickItem>

#include <Plasma/Corona>
#include <kactioncollection.h>
#include <QAction>

SystemTrayContainer::SystemTrayContainer(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
}

SystemTrayContainer::~SystemTrayContainer()
{
}

void SystemTrayContainer::init()
{
    Applet::init();
}

void SystemTrayContainer::constraintsEvent(Plasma::Types::Constraints constraints)
{
    if (constraints & Plasma::Types::LocationConstraint) {
        if (m_innerContainment) {
            m_innerContainment->setLocation(location());
        }
    }
    if (constraints & Plasma::Types::FormFactorConstraint) {
        if (m_innerContainment) {
            //m_innerContainment->setFormFactor(formFactor());
        }
    }
    if (constraints & Plasma::Types::StartupCompletedConstraint) {
        Plasma::Containment *cont = containment();
        if (!cont) {
            return;
        }

        Plasma::Corona *c = cont->corona();
        if (!c) {
            return;
        }

        uint id = config().readEntry("SystrayContainmentId", 0);
        qWarning()<<"CONTAINMENT ID"<<id;
        if (id > 0) {
            foreach (Plasma::Containment *candidate, c->containments()) {
                qWarning()<<"GGGG"<<candidate->id() << id;
                if (candidate->id() == id) {
                    qWarning()<<candidate;
                    m_innerContainment = candidate;
                    break;
                }
            }
            qWarning()<<"shouldn't go there";
            //id = 0;
        }
        qWarning()<<"FOUND ID:"<<id;
        if (id <= 0) {
            m_innerContainment = c->createContainment("org.kde.plasma.simplesystray");
            config().writeEntry("SystrayContainmentId", m_innerContainment->id());
        }

        if (!m_innerContainment) {
            return;
        }

        m_internalSystray = m_innerContainment->property("_plasma_graphicObject").value<QQuickItem *>();
        emit internalSystrayChanged();

        connect(m_innerContainment, &Plasma::Containment::configureRequested, this,
            [this] {
                emit containment()->configureRequested(m_innerContainment);
            }
        );
    }
}

QQuickItem *SystemTrayContainer::internalSystray()
{
    return m_internalSystray;
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemtraycontainer, SystemTrayContainer, "metadata.json")

#include "systemtraycontainer.moc"
