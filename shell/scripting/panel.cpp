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
    if (!c) {
        return 0;
    }

    return m_corona->panelView(c);
}

QString Panel::alignment() const
{
    PanelView *v = panel();
    if (!v) {
        return "left";
    }

    switch (v->alignment()) {
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
    PanelView *v = panel();
    if (v) {
        bool success = false;

        if (alignment.compare("left", Qt::CaseInsensitive) == 0) {
            success = true;
            v->setAlignment(Qt::AlignLeft);
        } else if (alignment.compare("right", Qt::CaseInsensitive) == 0) {
            success = true;
            v->setAlignment(Qt::AlignRight);
        } else if (alignment.compare("center", Qt::CaseInsensitive) == 0) {
            success = true;
            v->setAlignment(Qt::AlignCenter);
        }

        if (success) {
            v->setOffset(0);
        }
    }
}

int Panel::offset() const
{
    PanelView *v = panel();
    if (v) {
        return v->offset();
    }

    return 0;
}

void Panel::setOffset(int pixels)
{
    Plasma::Containment *c = containment();
    if (pixels < 0 || !c) {
        return;
    }

    QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

    if (!graphicObject) {
        return;
    }

    PanelView *v = panel();
    if (v) {
        QRectF screen = v->screen()->geometry();
        QSizeF size(graphicObject->width(), graphicObject->height());

        if (c->formFactor() == Plasma::Types::Vertical) {
            if (pixels > screen.height()) {
                return;
            }

            if (size.height() + pixels > screen.height()) {
                graphicObject->setWidth(size.width());
                graphicObject->setHeight(screen.height() - pixels);
            }
        } else if (pixels > screen.width()) {
            return;
        } else if (size.width() + pixels > screen.width()) {
            size.setWidth(screen.width() - pixels);
            graphicObject->setWidth(size.width());
            graphicObject->setHeight(size.height());
            v->setMinimumSize(size.toSize());
            v->setMaximumSize(size.toSize());
        }

        v->setOffset(pixels);
    }
}

int Panel::length() const
{
    Plasma::Containment *c = containment();
    if (!c) {
        return 0;
    }
    QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

    if (!graphicObject) {
        return 0;
    }

    if (c->formFactor() == Plasma::Types::Vertical) {
        return graphicObject->height();
    } else {
        return graphicObject->width();
    }
}

void Panel::setLength(int pixels)
{
    Plasma::Containment *c = containment();
    if (pixels < 0 || !c) {
        return;
    }

    QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

    if (!graphicObject) {
        return;
    }

    PanelView *v = panel();
    if (v) {
        QRectF screen = v->screen()->geometry();
        QSizeF s(graphicObject->width(), graphicObject->height());

        if (c->formFactor() == Plasma::Types::Vertical) {
            if (pixels > screen.height() - v->offset()) {
                return;
            }

            s.setHeight(pixels);
        } else if (pixels > screen.width() - v->offset()) {
            return;
        } else {
            s.setWidth(pixels);
        }
        v->setMinimumLength(pixels);
        v->setMaximumLength(pixels);
        v->setLength(pixels);
    }
}

int Panel::height() const
{
    Plasma::Containment *c = containment();
    if (!c) {
        return 0;
    }

    QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

    if (!graphicObject) {
        return 0;
    }

    return c->formFactor() == Plasma::Types::Vertical ? graphicObject->width()
                                               : graphicObject->height();
}

void Panel::setHeight(int height)
{
    Plasma::Containment *c = containment();
    if (height < 16 || !c) {
        return;
    }

    QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

    if (!graphicObject) {
        return;
    }

    PanelView *v = panel();
    if (v) {
        QRect screen = v->screen()->geometry();
        QSizeF size(graphicObject->width(), graphicObject->height());
        const int max = (c->formFactor() == Plasma::Types::Vertical ? screen.width() : screen.height()) / 3;
        height = qBound(16, height, max);

        v->setThickness(height);
    }
}

QString Panel::hiding() const
{
    /*PanelView *v = panel();
    if (v) {
        switch (v->visibilityMode()) {
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
    }*/

    return "none";
}

void Panel::setHiding(const QString &mode)
{
    /*PanelView *v = panel();
    if (v) {
        if (mode.compare("autohide", Qt::CaseInsensitive) == 0) {
            v->setVisibilityMode(PanelView::AutoHide);
        } else if (mode.compare("windowscover", Qt::CaseInsensitive) == 0) {
            v->setVisibilityMode(PanelView::LetWindowsCover);
        } else if (mode.compare("windowsbelow", Qt::CaseInsensitive) == 0) {
            v->setVisibilityMode(PanelView::WindowsGoBelow);
        } else {
            v->setVisibilityMode(PanelView::NormalPanel);
        }
    }*/
}

}

#include "panel.moc"

