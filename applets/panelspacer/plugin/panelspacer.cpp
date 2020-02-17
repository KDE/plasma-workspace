/***************************************************************************
 *   Copyright (C) 2020 Marco Martin <mart@kde.org>                        *
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

#include "panelspacer.h"

#include <QDebug>
#include <QProcess>
#include <QtQml>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <PlasmaQuick/AppletQuickItem>

class SpacersTrackerSingleton
{
public:
    SpacersTracker self;
};

Q_GLOBAL_STATIC(SpacersTrackerSingleton, privateSpacersTrackerSelf)

SpacersTracker::SpacersTracker(QObject *parent)
    : QObject(parent)
{
}

SpacersTracker::~SpacersTracker()
{
}

SpacersTracker *SpacersTracker::self()
{
    return &privateSpacersTrackerSelf()->self;
}

void SpacersTracker::insertSpacer(Plasma::Containment *containment, PanelSpacer *spacer)
{
    const bool wasTwin = m_spacers[containment].count() == 2;
    m_spacers[containment] << (spacer);
    const bool isTwin = m_spacers[containment].count() == 2;

    if (isTwin) {
        auto *lay1 = m_spacers[containment].first();
        auto *lay2 = m_spacers[containment].last();
        lay1->setTwinSpacer(lay2->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>());
        lay2->setTwinSpacer(lay1->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>());
    } else if (wasTwin) {
        for (auto *lay : m_spacers[containment]) {
            lay->setTwinSpacer(nullptr);
        }
    }
}

void SpacersTracker::removeSpacer(Plasma::Containment *containment, PanelSpacer *spacer)
{
    const bool wasTwin = m_spacers[containment].count() == 2;
    m_spacers[containment].removeAll(spacer);
    const bool isTwin = m_spacers[containment].count() == 2;

    if (isTwin) {
        auto *lay1 = m_spacers[containment].first();
        auto *lay2 = m_spacers[containment].last();
        lay1->setTwinSpacer(lay2->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>());
        lay2->setTwinSpacer(lay1->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>());

    } else if (wasTwin) {
        for (auto *lay : m_spacers[containment]) {
            lay->setTwinSpacer(nullptr);
        }
    }

    if (m_spacers[containment].isEmpty()) {
        m_spacers.remove(containment);
    }
}

/////////////////////////////////////////////////////////////////////

PanelSpacer::PanelSpacer(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
}

PanelSpacer::~PanelSpacer()
{
    SpacersTracker::self()->removeSpacer(containment(), this);
}

void PanelSpacer::init()
{
    
}

void PanelSpacer::constraintsEvent(Plasma::Types::Constraints constraints)
{
    // At this point we're sure the AppletQuickItem has been created already
    if (constraints & Plasma::Types::UiReadyConstraint) {
        Q_ASSERT(containment());
        Q_ASSERT(containment()->corona());

        SpacersTracker::self()->insertSpacer(containment(), this);
    }

    Plasma::Applet::constraintsEvent(constraints);
}

void PanelSpacer::setTwinSpacer(PlasmaQuick::AppletQuickItem *spacer)
{
    if (m_twinSpacer == spacer) {
        return;
    }

    m_twinSpacer = spacer;
    emit twinSpacerChanged();
}

PlasmaQuick::AppletQuickItem *PanelSpacer::twinSpacer() const
{
    return m_twinSpacer;
}

PlasmaQuick::AppletQuickItem *PanelSpacer::containmentGraphicObject() const
{
    return containment()->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();
}


K_EXPORT_PLASMA_APPLET_WITH_JSON(panelspacer, PanelSpacer, "metadata.json")

#include "panelspacer.moc"
