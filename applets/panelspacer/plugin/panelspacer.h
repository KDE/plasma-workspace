/***************************************************************************
 *   Copyright (C) 2020 Marco Martin <mart@kde.org>                        *
 *
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

#pragma once


#include <Plasma/Applet>

namespace PlasmaQuick {
    class AppletQuickItem;
}

namespace Plasma {
    class Containment;
}

class PanelSpacer;

class SpacersTracker : public QObject
{
    Q_OBJECT

public:
    SpacersTracker( QObject *parent = nullptr );
    ~SpacersTracker() override;

    static SpacersTracker *self();

    void insertSpacer(Plasma::Containment *containment, PanelSpacer *spacer);
    void removeSpacer(Plasma::Containment *containment, PanelSpacer *spacer);

private:
    QHash<Plasma::Containment *, QList<PanelSpacer *> > m_spacers;
};

class PanelSpacer : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(PlasmaQuick::AppletQuickItem *twinSpacer READ twinSpacer NOTIFY twinSpacerChanged)
    Q_PROPERTY(PlasmaQuick::AppletQuickItem *containment READ containmentGraphicObject CONSTANT)

public:
    PanelSpacer( QObject *parent, const QVariantList &args );
    ~PanelSpacer() override;

    void init() override;
    void constraintsEvent(Plasma::Types::Constraints constraints) override;

    void setTwinSpacer(PlasmaQuick::AppletQuickItem *spacer);
    PlasmaQuick::AppletQuickItem *twinSpacer() const;

    PlasmaQuick::AppletQuickItem *containmentGraphicObject() const;

Q_SIGNALS:
    void twinSpacerChanged();

private:
    PlasmaQuick::AppletQuickItem *m_twinSpacer;
};


