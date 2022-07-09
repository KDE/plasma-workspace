/*
    SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "panelspacer.h"

#include <QDebug>
#include <QProcess>
#include <QtQml>

#include <Plasma/Containment>
#include <Plasma/Corona>

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

PanelSpacer::PanelSpacer(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
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
    Q_EMIT twinSpacerChanged();
}

PlasmaQuick::AppletQuickItem *PanelSpacer::twinSpacer() const
{
    return m_twinSpacer;
}

PlasmaQuick::AppletQuickItem *PanelSpacer::containmentGraphicObject() const
{
    if (!containment()) return nullptr; // Return nothing if there is no containment to prevent a Segmentation Fault
    return containment()->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();
}

K_PLUGIN_CLASS(PanelSpacer)

#include "panelspacer.moc"
