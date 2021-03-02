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

#include <QQuickItem>

#include <Plasma/Corona>
#include <QAction>
#include <kactioncollection.h>

SystemTrayContainer::SystemTrayContainer(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
}

SystemTrayContainer::~SystemTrayContainer()
{
    if (destroyed()) {
        m_innerContainment->destroy();
    }
}

void SystemTrayContainer::init()
{
    Applet::init();

    // in the first creation we immediately create the systray: so it's accessible during desktop scripting
    uint id = config().readEntry("SystrayContainmentId", 0);

    if (id == 0) {
        ensureSystrayExists();
    }
}

void SystemTrayContainer::ensureSystrayExists()
{
    if (m_innerContainment) {
        return;
    }

    Plasma::Containment *cont = containment();
    if (!cont) {
        return;
    }

    Plasma::Corona *c = cont->corona();
    if (!c) {
        return;
    }

    uint id = config().readEntry("SystrayContainmentId", 0);
    if (id > 0) {
        foreach (Plasma::Containment *candidate, c->containments()) {
            if (candidate->id() == id) {
                m_innerContainment = candidate;
                break;
            }
        }
        qCDebug(SYSTEM_TRAY_CONTAINER) << "Containment id" << id << "that used to be a system tray was deleted";
        // id = 0;
    }

    if (!m_innerContainment) {
        m_innerContainment = c->createContainment(QStringLiteral("org.kde.plasma.private.systemtray"), QVariantList() << "org.kde.plasma:force-create");
        config().writeEntry("SystrayContainmentId", m_innerContainment->id());
    }

    if (!m_innerContainment) {
        return;
    }

    m_innerContainment->setParent(this);
    connect(containment(), &Plasma::Containment::screenChanged, m_innerContainment.data(), &Plasma::Containment::reactToScreenChange);

    if (formFactor() == Plasma::Types::Horizontal || formFactor() == Plasma::Types::Vertical) {
        m_innerContainment->setFormFactor(formFactor());
    } else {
        m_innerContainment->setFormFactor(Plasma::Types::Horizontal);
    }

    if (m_innerContainment) {
        m_innerContainment->setLocation(location());
    }

    m_internalSystray = m_innerContainment->property("_plasma_graphicObject").value<QQuickItem *>();
    emit internalSystrayChanged();

    actions()->addAction("configure", m_innerContainment->actions()->action("configure"));
    connect(m_innerContainment.data(), &Plasma::Containment::configureRequested, this, [this](Plasma::Applet *applet) {
        emit containment()->configureRequested(applet);
    });

    if (m_internalSystray) {
        // don't let internal systray manage context menus
        m_internalSystray->setAcceptedMouseButtons(Qt::NoButton);
    }

    // replace internal remove action with ours
    m_innerContainment->actions()->addAction("remove", actions()->action("remove"));

    //Sync the display hints
    m_innerContainment->setContainmentDisplayHints(containmentDisplayHints() | Plasma::Types::ContainmentDrawsPlasmoidHeading | Plasma::Types::ContainmentForcesSquarePlasmoids);
    connect(cont, &Plasma::Containment::containmentDisplayHintsChanged, this,
        [this]() {
            m_innerContainment->setContainmentDisplayHints(containmentDisplayHints() | Plasma::Types::ContainmentDrawsPlasmoidHeading | Plasma::Types::ContainmentForcesSquarePlasmoids);
        }
    );
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
            if (formFactor() == Plasma::Types::Horizontal || formFactor() == Plasma::Types::Vertical) {
                m_innerContainment->setFormFactor(formFactor());
            } else {
                m_innerContainment->setFormFactor(Plasma::Types::Horizontal);
            }
        }
    }

    if (constraints & Plasma::Types::UiReadyConstraint) {
        ensureSystrayExists();
    }
}

QQuickItem *SystemTrayContainer::internalSystray()
{
    return m_internalSystray;
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemtraycontainer, SystemTrayContainer, "metadata.json")

#include "systemtraycontainer.moc"
