/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "panel.h"

#include <QAction>
#include <QQuickItem>

#include <Plasma/Corona>
#include <Plasma/Containment>
#include <QScreen>

#include "shellcorona.h"
#include "panelview.h"
#include "scriptengine.h"
#include "widget.h"

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
        return "floating";
    }

    switch (c->location()) {
        case Plasma::Types::Floating:
            return "floating";
        case Plasma::Types::Desktop:
            return "desktop";
        case Plasma::Types::FullScreen:
            return "fullscreen";
        case Plasma::Types::TopEdge:
            return "top";
        case Plasma::Types::BottomEdge:
            return "bottom";
        case Plasma::Types::LeftEdge:
            return "left";
        case Plasma::Types::RightEdge:
            return "right";
    }

    return "floating";
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
    if (lower == "desktop") {
        loc = Plasma::Types::Desktop;
    } else if (lower == "fullscreen") {
        loc = Plasma::Types::FullScreen;
    } else if (lower == "top") {
        loc = Plasma::Types::TopEdge;
        ff = Plasma::Types::Horizontal;
    } else if (lower == "bottom") {
        loc = Plasma::Types::BottomEdge;
        ff = Plasma::Types::Horizontal;
    } else if (lower == "left") {
        loc = Plasma::Types::LeftEdge;
        ff = Plasma::Types::Vertical;
    } else if (lower == "right") {
        loc = Plasma::Types::RightEdge;
        ff = Plasma::Types::Vertical;
    }

    c->setLocation(loc);
    c->setFormFactor(ff);
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
    int screenNum = qMax(screen(), 0); //if we don't have a screen (-1) we'll be put on screen 0

    if (QGuiApplication::screens().size() < screenNum) {
        return KConfigGroup();
    }
    QScreen *s = QGuiApplication::screens().at(screenNum);
    return PanelView::panelConfig(corona(), containment(), s);
}

//NOTE: when we don't have a view we should write only to the defaults group as we don't know yet during startup if we are on the "final" screen resolution yet
KConfigGroup Panel::panelConfigDefaults() const
{
    int screenNum = qMax(screen(), 0); //if we don't have a screen (-1) we'll be put on screen 0

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
            return "right";
        case Qt::AlignCenter:
            return "center";
        default:
            return "left";
    }

    return "left";
}

// NOTE: Alignment is the only one that reads and writes directly from panelconfig()
void Panel::setAlignment(const QString &alignment)
{
    int a = Qt::AlignLeft;
    if (alignment.compare("right", Qt::CaseInsensitive) == 0) {
        a = Qt::AlignRight;
    } else if (alignment.compare("center", Qt::CaseInsensitive) == 0) {
        a = Qt::AlignCenter;
    }

    // Always prefer the view, if available
    if (panel()) {
        panel()->setAlignment(Qt::Alignment(a));
    } else {
        panelConfig().writeEntry("alignment", a);
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
            return "none";
        case PanelView::AutoHide:
            return "autohide";
        case PanelView::LetWindowsCover:
            return "windowscover";
        case PanelView::WindowsGoBelow:
            return "windowsbelow";
    }
    return "none";
}

void Panel::setHiding(const QString &mode)
{
    PanelView::VisibilityMode visibilityMode = PanelView::NormalPanel;
    if (mode.compare("autohide", Qt::CaseInsensitive) == 0) {
        visibilityMode = PanelView::AutoHide;
    } else if (mode.compare("windowscover", Qt::CaseInsensitive) == 0) {
        visibilityMode = PanelView::LetWindowsCover;
    } else if (mode.compare("windowsbelow", Qt::CaseInsensitive) == 0) {
        visibilityMode = PanelView::WindowsGoBelow;
    }

    if (panel()) {
        panel()->setVisibilityMode(visibilityMode);
    } else {
        panelConfig().writeEntry("panelVisibility", (int)visibilityMode);
    }
}

}



