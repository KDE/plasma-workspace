/*
    SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Applet>
#include <PlasmaQuick/AppletQuickItem>

namespace Plasma
{
class Containment;
}

class PanelSpacer;

class SpacersTracker : public QObject
{
    Q_OBJECT

public:
    SpacersTracker(QObject *parent = nullptr);
    ~SpacersTracker() override;

    static SpacersTracker *self();

    void insertSpacer(Plasma::Containment *containment, PanelSpacer *spacer);
    void removeSpacer(Plasma::Containment *containment, PanelSpacer *spacer);

private:
    QHash<Plasma::Containment *, QList<PanelSpacer *>> m_spacers;
};

class PanelSpacer : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(PlasmaQuick::AppletQuickItem *twinSpacer READ twinSpacer NOTIFY twinSpacerChanged)
    Q_PROPERTY(PlasmaQuick::AppletQuickItem *containment READ containmentGraphicObject CONSTANT)

public:
    PanelSpacer(QObject *parent, const QVariantList &args);
    ~PanelSpacer() override;

    void init() override;
    void constraintsEvent(Plasma::Types::Constraints constraints) override;

    void setTwinSpacer(PlasmaQuick::AppletQuickItem *spacer);
    PlasmaQuick::AppletQuickItem *twinSpacer() const;

    PlasmaQuick::AppletQuickItem *containmentGraphicObject() const;

Q_SIGNALS:
    void twinSpacerChanged();

private:
    PlasmaQuick::AppletQuickItem *m_twinSpacer = nullptr;
};
