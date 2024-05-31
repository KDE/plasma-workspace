/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "panel.h"

#include <QAction>
#include <QQuickItem>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <QScreen>

#include "panelview.h"
#include "scriptengine.h"
#include "shellcorona.h"
#include "widget.h"

using namespace Qt::StringLiterals;

namespace WorkspaceScripting
{
Panel::Panel(Plasma::Containment *containment, ScriptEngine *engine)
    : Containment(containment, engine)
{
}

Panel::~Panel()
{
}

QString Panel::location() const
{
    Plasma::Containment *c = containment();
    if (!c) {
        return u"floating"_s;
    }

    switch (c->location()) {
    case Plasma::Types::Floating:
        return u"floating"_s;
    case Plasma::Types::Desktop:
        return u"desktop"_s;
    case Plasma::Types::FullScreen:
        return u"fullscreen"_s;
    case Plasma::Types::TopEdge:
        return u"top"_s;
    case Plasma::Types::BottomEdge:
        return u"bottom"_s;
    case Plasma::Types::LeftEdge:
        return u"left"_s;
    case Plasma::Types::RightEdge:
        return u"right"_s;
    }

    return u"floating"_s;
}

void Panel::setLocation(const QString &locationString)
{
    Plasma::Containment *c = containment();
    if (!c) {
        return;
    }

    const QString lower = locationString.toLower();
    Plasma::Types::Location loc = Plasma::Types::Floating;
    Plasma::Types::FormFactor ff = Plasma::Types::Planar;
    if (lower == u"desktop") {
        loc = Plasma::Types::Desktop;
    } else if (lower == u"fullscreen") {
        loc = Plasma::Types::FullScreen;
    } else if (lower == u"top") {
        loc = Plasma::Types::TopEdge;
        ff = Plasma::Types::Horizontal;
    } else if (lower == u"bottom") {
        loc = Plasma::Types::BottomEdge;
        ff = Plasma::Types::Horizontal;
    } else if (lower == u"left") {
        loc = Plasma::Types::LeftEdge;
        ff = Plasma::Types::Vertical;
    } else if (lower == u"right") {
        loc = Plasma::Types::RightEdge;
        ff = Plasma::Types::Vertical;
    }

    c->setLocation(loc);
    c->setFormFactor(ff);

    /*
     * After location is set, one layout script will usually start adding widgets.
     * It's required to emit formFactorChanged() to update plasmoid.formFactor bindings
     * in QML side to avoid Layout issues.
     *
     * @see isHorizontal in plasma-desktop/containments/panel/contents/ui/main.qml
     */
    c->flushPendingConstraintsEvents();
}

PanelView *Panel::panel() const
{
    Plasma::Containment *c = containment();
    if (!c || !corona()) {
        return nullptr;
    }

    return corona()->panelView(c);
}

// NOTE: this is used *only* for alignment and visibility
KConfigGroup Panel::panelConfig() const
{
    int screenNum = qMax(screen(), 0); // if we don't have a screen (-1) we'll be put on screen 0

    if (QGuiApplication::screens().size() < screenNum) {
        return KConfigGroup();
    }
    QScreen *s = QGuiApplication::screens().at(screenNum);
    return PanelView::panelConfig(corona(), containment(), s);
}

// NOTE: when we don't have a view we should write only to the defaults group as we don't know yet during startup if we are on the "final" screen resolution yet
KConfigGroup Panel::panelConfigDefaults() const
{
    int screenNum = qMax(screen(), 0); // if we don't have a screen (-1) we'll be put on screen 0

    if (QGuiApplication::screens().size() < screenNum) {
        return KConfigGroup();
    }
    QScreen *s = QGuiApplication::screens().at(screenNum);
    return PanelView::panelConfigDefaults(corona(), containment(), s);
}

// NOTE: Alignment is the only one that reads and writes directly from panelconfig()
QString Panel::alignment() const
{
    int alignment;
    if (panel()) {
        alignment = panel()->alignment();
    } else {
        alignment = panelConfig().readEntry("alignment", 0);
    }

    switch (alignment) {
    case Qt::AlignRight:
        return u"right"_s;
    case Qt::AlignCenter:
        return u"center"_s;
    default:
        return u"left"_s;
    }
}

// NOTE: Alignment is the only one that reads and writes directly from panelconfig()
void Panel::setAlignment(const QString &alignment)
{
    int a = Qt::AlignCenter;
    if (alignment.compare(u"left", Qt::CaseInsensitive) == 0) {
        a = Qt::AlignLeft;
    } else if (alignment.compare(u"right", Qt::CaseInsensitive) == 0) {
        a = Qt::AlignRight;
    }

    // Always prefer the view, if available
    if (panel()) {
        panel()->setAlignment(Qt::Alignment(a));
    } else {
        panelConfig().writeEntry("alignment", a);
    }
}

// NOTE: lengthMode also reads and writes directly from panelConfig() because it
// is resolution independent
QString Panel::lengthMode() const
{
    int lengthMode;
    if (panel()) {
        lengthMode = panel()->lengthMode();
    } else {
        lengthMode = panelConfig().readEntry("panelLengthMode", 0);
    }

    switch (lengthMode) {
    case PanelView::LengthMode::FillAvailable:
        return u"fill"_s;
    case PanelView::LengthMode::FitContent:
        return u"fit"_s;
    case PanelView::LengthMode::Custom:
        return u"custom"_s;
    }

    return u"fill"_s;
}

void Panel::setLengthMode(const QString &mode)
{
    PanelView::LengthMode lengthMode = PanelView::LengthMode::FillAvailable;
    if (mode.compare(u"fit", Qt::CaseInsensitive) == 0) {
        lengthMode = PanelView::LengthMode::FitContent;
    } else if (mode.compare(u"custom", Qt::CaseInsensitive) == 0) {
        lengthMode = PanelView::LengthMode::Custom;
    }

    if (panel()) {
        panel()->setLengthMode(lengthMode);
    } else {
        panelConfig().writeEntry("panelLengthMode", (int)lengthMode);
    }
}

// From now on only panelConfigDefaults should be used
int Panel::offset() const
{
    if (panel()) {
        return panel()->offset();
    } else {
        return panelConfigDefaults().readEntry("offset", 0);
    }
}

void Panel::setOffset(int pixels)
{
    panelConfigDefaults().writeEntry("offset", pixels);
    if (panel()) {
        panel()->setOffset(pixels);
    } else {
        panelConfigDefaults().readEntry("offset", pixels);
    }
}

int Panel::length() const
{
    if (panel()) {
        return panel()->length();
    } else {
        return panelConfigDefaults().readEntry("length", 0);
    }
}

void Panel::setLength(int pixels)
{
    if (panel()) {
        panel()->setLength(pixels);
    } else {
        panelConfigDefaults().writeEntry("length", pixels);
    }
}

int Panel::minimumLength() const
{
    if (panel()) {
        return panel()->minimumLength();
    } else {
        return panelConfigDefaults().readEntry("minLength", 0);
    }
}

void Panel::setMinimumLength(int pixels)
{
    if (panel()) {
        panel()->setMinimumLength(pixels);
    } else {
        panelConfigDefaults().writeEntry("minLength", pixels);
    }
}

int Panel::maximumLength() const
{
    if (panel()) {
        return panel()->maximumLength();
    } else {
        return panelConfigDefaults().readEntry("maxLength", 0);
    }
}

void Panel::setMaximumLength(int pixels)
{
    if (panel()) {
        panel()->setMaximumLength(pixels);
    } else {
        panelConfigDefaults().writeEntry("maxLength", pixels);
    }
}

int Panel::height() const
{
    if (panel()) {
        return panel()->thickness();
    } else {
        return panelConfigDefaults().readEntry("thickness", 0);
    }
}

void Panel::setHeight(int height)
{
    if (panel()) {
        panel()->setThickness(height);
    } else {
        panelConfigDefaults().writeEntry("thickness", height);
    }
}

QString Panel::hiding() const
{
    int visibility;
    if (panel()) {
        visibility = panel()->visibilityMode();
    } else {
        visibility = panelConfig().readEntry("panelVisibility", 0);
    }

    switch (visibility) {
    case PanelView::NormalPanel:
        return u"none"_s;
    case PanelView::AutoHide:
        return u"autohide"_s;
    case PanelView::DodgeWindows:
        return u"dodgewindows"_s;
    }
    return u"none"_s;
}

void Panel::setHiding(const QString &mode)
{
    PanelView::VisibilityMode visibilityMode = PanelView::NormalPanel;
    if (mode.compare(u"autohide", Qt::CaseInsensitive) == 0) {
        visibilityMode = PanelView::AutoHide;
    }
    if (mode.compare(u"dodgewindows", Qt::CaseInsensitive) == 0) {
        visibilityMode = PanelView::DodgeWindows;
    }
    if (mode.compare(u"windowsgobelow", Qt::CaseInsensitive) == 0) {
        visibilityMode = PanelView::WindowsGoBelow;
    }

    if (panel()) {
        panel()->setVisibilityMode(visibilityMode);
    } else {
        panelConfig().writeEntry("panelVisibility", (int)visibilityMode);
    }
}

bool Panel::floating() const
{
    if (panel()) {
        return panel()->floating();
    } else {
        return panelConfig().readEntry("floating", true);
    }
}

void Panel::setFloating(bool floating)
{
    if (panel()) {
        panel()->setFloating(floating);
    } else {
        panelConfig().writeEntry("floating", (int)floating);
    }
}
}
