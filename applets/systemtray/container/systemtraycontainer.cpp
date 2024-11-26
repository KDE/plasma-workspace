/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemtraycontainer.h"
#include "debug.h"

#include <Plasma/Corona>
#include <PlasmaQuick/AppletQuickItem>
#include <QAction>
#include <kactioncollection.h>

using namespace Qt::StringLiterals;

SystemTrayContainer::SystemTrayContainer(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
}

SystemTrayContainer::~SystemTrayContainer()
{
    // If we are actually destroyed (to get rid of config file entry)
    // properly destoy the inner systrayu as well, to not leave orphaned
    // systrays in the config file
    if (destroyed() && m_innerContainment) {
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
        for (const auto conts = c->containments(); Plasma::Containment * candidate : conts) {
            if (candidate->id() == id) {
                m_innerContainment = candidate;
                break;
            }
        }
        qCDebug(SYSTEM_TRAY_CONTAINER) << "Containment id" << id << "that used to be a system tray was deleted";
        // id = 0;
    }

    if (!m_innerContainment) {
        m_innerContainment = c->createContainment(QStringLiteral("org.kde.plasma.private.systemtray"), QVariantList() << u"org.kde.plasma:force-create"_s);
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

    auto oldInternalSystray = m_internalSystray;
    m_internalSystray = PlasmaQuick::AppletQuickItem::itemForApplet(m_innerContainment);

    if (m_internalSystray != oldInternalSystray) {
        Q_EMIT internalSystrayChanged();
    }

    setInternalAction(u"configure"_s, m_innerContainment->internalAction(u"configure"_s));
    connect(m_innerContainment.data(), &Plasma::Containment::configureRequested, this, [this](Plasma::Applet *applet) {
        Q_EMIT(containment()->configureRequested(applet));
    });

    if (m_internalSystray) {
        // don't let internal systray manage context menus
        m_internalSystray->setAcceptedMouseButtons(Qt::NoButton);
    }

    // replace internal remove action with ours
    m_innerContainment->setInternalAction(u"remove"_s, internalAction(u"remove"_s));

    // Sync the display hints
    m_innerContainment->setContainmentDisplayHints(containmentDisplayHints() | Plasma::Types::ContainmentDrawsPlasmoidHeading
                                                   | Plasma::Types::ContainmentForcesSquarePlasmoids);
    connect(cont, &Plasma::Containment::containmentDisplayHintsChanged, this, [this]() {
        m_innerContainment->setContainmentDisplayHints(containmentDisplayHints() | Plasma::Types::ContainmentDrawsPlasmoidHeading
                                                       | Plasma::Types::ContainmentForcesSquarePlasmoids);
    });
}

void SystemTrayContainer::constraintsEvent(Plasma::Applet::Constraints constraints)
{
    if (constraints & Plasma::Applet::LocationConstraint) {
        if (m_innerContainment) {
            m_innerContainment->setLocation(location());
        }
    }

    if (constraints & Plasma::Applet::FormFactorConstraint) {
        if (m_innerContainment) {
            if (formFactor() == Plasma::Types::Horizontal || formFactor() == Plasma::Types::Vertical) {
                m_innerContainment->setFormFactor(formFactor());
            } else {
                m_innerContainment->setFormFactor(Plasma::Types::Horizontal);
            }
        }
    }

    if (constraints & Plasma::Applet::UiReadyConstraint) {
        ensureSystrayExists();
        // At this point we are sure that all the contianments and all the applets are loaded
        cleanupConfig();
    }
}

QQuickItem *SystemTrayContainer::internalSystray()
{
    return m_internalSystray;
}

void SystemTrayContainer::cleanupConfig()
{
    // In the past inner systrays were leaked in the config file,
    // and users might end up with tens or ever hundreds of systrays (and all their applets)
    // in the config file taking up resources and causing also  https://bugs.kde.org/show_bug.cgi?id=472937
    // Should be fine to do this every time as when there is nothing to cleanup the whole
    // cycle is usually under a millisecond

    Plasma::Containment *cont = containment();
    if (!cont) {
        return;
    }
    Plasma::Corona *c = cont->corona();
    if (!c) {
        return;
    }

    // All inner systray ids we found that are owned by a SystemTrayContainer
    QSet<int> ownedSystrays = {m_innerContainment->id()};

    // First search all SystemTrayContainer applets, and save the association with
    // internal systrays
    for (const auto conts = c->containments(); Plasma::Containment * containment : conts) {
        for (const auto applets = containment->applets(); Plasma::Applet * applet : applets) {
            if (SystemTrayContainer *contApplet = qobject_cast<SystemTrayContainer *>(applet)) {
                if (contApplet->m_innerContainment) {
                    ownedSystrays.insert(contApplet->m_innerContainment->id());
                } else {
                    const uint id = applet->config().readEntry("SystrayContainmentId", 0);
                    ownedSystrays.insert(id);
                }
            }
        }
    }

    // Destroy all systrays not owned by a SystemTrayContainer
    for (const auto conts = c->containments(); Plasma::Containment * containment : conts) {
        if (containment->pluginName() == u"org.kde.plasma.private.systemtray"_s) {
            if (!ownedSystrays.contains(containment->id())) {
                containment->destroy();
            }
        }
    }
}

K_PLUGIN_CLASS(SystemTrayContainer)

#include "systemtraycontainer.moc"

#include "moc_systemtraycontainer.cpp"
