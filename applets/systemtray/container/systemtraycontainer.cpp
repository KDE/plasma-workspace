/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemtraycontainer.h"
#include "debug.h"

#include <Plasma/Corona>
#include <QAction>
#include <kactioncollection.h>

SystemTrayContainer::SystemTrayContainer(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
}

SystemTrayContainer::~SystemTrayContainer()
{
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

    m_innerContainment->setLocation(location());

    m_internalSystray = m_innerContainment->property("_plasma_graphicObject").value<QQuickItem *>();
    Q_EMIT internalSystrayChanged();

    actions()->addAction("configure", m_innerContainment->actions()->action("configure"));
    connect(m_innerContainment.data(), &Plasma::Containment::configureRequested, this, [this](Plasma::Applet *applet) {
        Q_EMIT containment()->configureRequested(applet);
    });

    if (m_internalSystray) {
        // don't let internal systray manage context menus
        m_internalSystray->setAcceptedMouseButtons(Qt::NoButton);
    }

    // replace internal remove action with ours
    m_innerContainment->actions()->addAction("remove", actions()->action("remove"));

    // Sync the display hints
    m_innerContainment->setContainmentDisplayHints(containmentDisplayHints() | Plasma::Types::ContainmentDrawsPlasmoidHeading
                                                   | Plasma::Types::ContainmentForcesSquarePlasmoids);
    connect(cont, &Plasma::Containment::containmentDisplayHintsChanged, this, [this]() {
        m_innerContainment->setContainmentDisplayHints(containmentDisplayHints() | Plasma::Types::ContainmentDrawsPlasmoidHeading
                                                       | Plasma::Types::ContainmentForcesSquarePlasmoids);
    });
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

K_PLUGIN_CLASS(SystemTrayContainer)

#include "systemtraycontainer.moc"
