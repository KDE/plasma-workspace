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

Panel::Panel(Plasma::Containment *containment, QObject *parent)
    : Containment(containment, parent)
{
    m_corona = qobject_cast<ShellCorona *>(containment->corona());
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
            break;
        case Plasma::Types::Desktop:
            return "desktop";
            break;
        case Plasma::Types::FullScreen:
            return "fullscreen";
            break;
        case Plasma::Types::TopEdge:
            return "top";
            break;
        case Plasma::Types::BottomEdge:
            return "bottom";
            break;
        case Plasma::Types::LeftEdge:
            return "left";
            break;
        case Plasma::Types::RightEdge:
            return "right";
            break;
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
    if (!c || !m_corona) {
        return 0;
    }

    return m_corona->panelView(c);
}


KConfigGroup Panel::panelConfig() const
{
    int screenNum = qMax(screen(), 0); //if we don't have a screen (-1) we'll be put on screen 0

    if (QGuiApplication::screens().size() < screenNum) {
        return KConfigGroup();
    }
    QScreen *s = QGuiApplication::screens().at(screenNum);
    return PanelView::panelConfig(m_corona, containment(), s);
}

QString Panel::alignment() const
{
    switch (panelConfig().readEntry("alignment", 0)) {
        case Qt::AlignRight:
            return "right";
            break;
        case Qt::AlignCenter:
            return "center";
            break;
        default:
            return "left";
            break;
    }

    return "left";
}

void Panel::setAlignment(const QString &alignment)
{
    int a = Qt::AlignLeft;
    if (alignment.compare("right", Qt::CaseInsensitive) == 0) {
        a = Qt::AlignRight;
    } else if (alignment.compare("center", Qt::CaseInsensitive) == 0) {
        a = Qt::AlignCenter;
    }

    panelConfig().writeEntry("alignment", a);
    if (panel()) {
        QMetaObject::invokeMethod(panel(), "restore");
    }
}

int Panel::offset() const
{
    return panelConfig().readEntry("offset", 0);
}

void Panel::setOffset(int pixels)
{
   panelConfig().writeEntry("offset", pixels);
   if (panel()) {
       QMetaObject::invokeMethod(panel(), "restore");
   }
}

int Panel::length() const
{
    return panelConfig().readEntry("length", 0);
}

void Panel::setLength(int pixels)
{
   panelConfig().writeEntry("length", pixels);
   if (panel()) {
       QMetaObject::invokeMethod(panel(), "restore");
   }
}

int Panel::minimumLength() const
{
    return panelConfig().readEntry("minLength", 0);
}

void Panel::setMinimumLength(int pixels)
{
   panelConfig().writeEntry("minLength", pixels);
   if (panel()) {
       QMetaObject::invokeMethod(panel(), "restore");
   }
}

int Panel::maximumLength() const
{
    return panelConfig().readEntry("maxLength", 0);
}

void Panel::setMaximumLength(int pixels)
{
   panelConfig().writeEntry("maxLength", pixels);
   if (panel()) {
       QMetaObject::invokeMethod(panel(), "restore");
   }
}

int Panel::height() const
{
    return panelConfig().readEntry("thickness", 0);
}

void Panel::setHeight(int height)
{
    panelConfig().writeEntry("thickness", height);
    if (panel()) {
        QMetaObject::invokeMethod(panel(), "restore");
    }
}

QString Panel::hiding() const
{
    switch (panelConfig().readEntry("panelVisibility", 0)) {
        case PanelView::NormalPanel:
            return "none";
            break;
        case PanelView::AutoHide:
            return "autohide";
            break;
        case PanelView::LetWindowsCover:
            return "windowscover";
            break;
        case PanelView::WindowsGoBelow:
            return "windowsbelow";
            break;
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

    panelConfig().writeEntry("panelVisibility", (int)visibilityMode);
    if (panel()) {
        QMetaObject::invokeMethod(panel(), "restore");
    }
}

}



