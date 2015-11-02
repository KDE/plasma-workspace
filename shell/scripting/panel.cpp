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
        return QStringLiteral("floating");
    }

    switch (c->location()) {
        case Plasma::Types::Floating:
            return QStringLiteral("floating");
            break;
        case Plasma::Types::Desktop:
            return QStringLiteral("desktop");
            break;
        case Plasma::Types::FullScreen:
            return QStringLiteral("fullscreen");
            break;
        case Plasma::Types::TopEdge:
            return QStringLiteral("top");
            break;
        case Plasma::Types::BottomEdge:
            return QStringLiteral("bottom");
            break;
        case Plasma::Types::LeftEdge:
            return QStringLiteral("left");
            break;
        case Plasma::Types::RightEdge:
            return QStringLiteral("right");
            break;
    }

    return QStringLiteral("floating");
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
    if (lower == QLatin1String("desktop")) {
        loc = Plasma::Types::Desktop;
    } else if (lower == QLatin1String("fullscreen")) {
        loc = Plasma::Types::FullScreen;
    } else if (lower == QLatin1String("top")) {
        loc = Plasma::Types::TopEdge;
        ff = Plasma::Types::Horizontal;
    } else if (lower == QLatin1String("bottom")) {
        loc = Plasma::Types::BottomEdge;
        ff = Plasma::Types::Horizontal;
    } else if (lower == QLatin1String("left")) {
        loc = Plasma::Types::LeftEdge;
        ff = Plasma::Types::Vertical;
    } else if (lower == QLatin1String("right")) {
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

QString Panel::alignment() const
{
    PanelView *v = panel();
    if (!v) {
        return QStringLiteral("left");
    }

    switch (v->alignment()) {
        case Qt::AlignRight:
            return QStringLiteral("right");
            break;
        case Qt::AlignCenter:
            return QStringLiteral("center");
            break;
        default:
            return QStringLiteral("left");
            break;
    }

    return QStringLiteral("left");
}

void Panel::setAlignment(const QString &alignment)
{
    Qt::Alignment alignmentValue = Qt::AlignLeft;

    bool success = false;

    if (alignment.compare(QLatin1String("left"), Qt::CaseInsensitive) == 0) {
        success = true;
        alignmentValue = Qt::AlignLeft;
    } else if (alignment.compare(QLatin1String("right"), Qt::CaseInsensitive) == 0) {
        success = true;
        alignmentValue = Qt::AlignRight;
    } else if (alignment.compare(QLatin1String("center"), Qt::CaseInsensitive) == 0) {
        success = true;
        alignmentValue = Qt::AlignCenter;
    }

    if (!success) {
        return;
    }

    PanelView *v = panel();
    if (v) {
        v->setOffset(0);
        v->setAlignment(alignmentValue);
    } else {
        QQuickItem *graphicObject = qobject_cast<QQuickItem *>(containment()->property("_plasma_graphicObject").value<QObject *>());

        if (!graphicObject) {
            return;
        }
        graphicObject->setProperty("_plasma_desktopscripting_alignment", (int)alignmentValue);
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
    PanelView *v = panel();
    Plasma::Containment *c = containment();

    if (!v || !c || pixels < 0 || pixels > v->maximumLength() || pixels < v->minimumLength()) {
        return;
    }

    QRectF screen = v->screen()->geometry();

    if (c->formFactor() == Plasma::Types::Vertical
        && pixels > screen.height() - v->offset()) {
        return;
    } else if (pixels > screen.width() - v->offset()) {
        return;
    }

    v->setLength(pixels);
}

int Panel::minimumLength() const
{
    PanelView *v = panel();

    if (v) {
        return v->minimumLength();
    }

    return 0;
}

void Panel::setMinimumLength(int pixels)
{
    PanelView *v = panel();
    Plasma::Containment *c = containment();

    if (!c || pixels < 0) {
        return;
    }

    //NOTE: due to dependency of class creation at startup, we can't in any way have
    //the panel views already instantiated, so put the property in a placeholder dynamic property
    if (!v) {
        QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

        if (!graphicObject) {
            return;
        }
        graphicObject->setProperty("_plasma_desktopscripting_minLength", pixels);
        return;
    }

    QRectF screen = v->screen()->geometry();

    if (c->formFactor() == Plasma::Types::Vertical
        && pixels > screen.height() - v->offset()) {
        return;
    } else if (pixels > screen.width() - v->offset()) {
        return;
    }

    v->setMinimumLength(pixels);

    if (v->maximumLength() < pixels) {
        v->setMaximumLength(pixels);
    }
}

int Panel::maximumLength() const
{
    PanelView *v = panel();

    if (v) {
        return v->maximumLength();
    }

    return 0;
}

void Panel::setMaximumLength(int pixels)
{
    PanelView *v = panel();
    Plasma::Containment *c = containment();

    if (!c || pixels < 0) {
        return;
    }

    //NOTE: due to dependency of class creation at startup, we can't in any way have
    //the panel views already instantiated, so put the property in a placeholder dynamic property
    if (!v) {
        QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

        if (!graphicObject) {
            return;
        }
        graphicObject->setProperty("_plasma_desktopscripting_maxLength", pixels);
        return;
    }

    QRectF screen = v->screen()->geometry();

    if (c->formFactor() == Plasma::Types::Vertical
        && pixels > screen.height() - v->offset()) {
        return;
    } else if (pixels > screen.width() - v->offset()) {
        return;
    }

    v->setMaximumLength(pixels);

    if (v->minimumLength() > pixels) {
        v->setMinimumLength(pixels);
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
    } else {
        //NOTE: due to dependency of class creation at startup, we can't in any way have
        //the panel views already instantiated, so put the property in a placeholder dynamic property
        QQuickItem *graphicObject = qobject_cast<QQuickItem *>(c->property("_plasma_graphicObject").value<QObject *>());

        if (!graphicObject) {
            return;
        }
        graphicObject->setProperty("_plasma_desktopscripting_thickness", height);
    }
}

QString Panel::hiding() const
{
    PanelView *v = panel();
    if (v) {
        switch (v->visibilityMode()) {
            case PanelView::NormalPanel:
                return QStringLiteral("none");
                break;
            case PanelView::AutoHide:
                return QStringLiteral("autohide");
                break;
            case PanelView::LetWindowsCover:
                return QStringLiteral("windowscover");
                break;
            case PanelView::WindowsGoBelow:
                return QStringLiteral("windowsbelow");
                break;
        }
    }

    return QStringLiteral("none");
}

void Panel::setHiding(const QString &mode)
{
    PanelView *v = panel();
    if (v) {
        if (mode.compare(QLatin1String("autohide"), Qt::CaseInsensitive) == 0) {
            v->setVisibilityMode(PanelView::AutoHide);
        } else if (mode.compare(QLatin1String("windowscover"), Qt::CaseInsensitive) == 0) {
            v->setVisibilityMode(PanelView::LetWindowsCover);
        } else if (mode.compare(QLatin1String("windowsbelow"), Qt::CaseInsensitive) == 0) {
            v->setVisibilityMode(PanelView::WindowsGoBelow);
        } else {
            v->setVisibilityMode(PanelView::NormalPanel);
        }
    }
}

}



