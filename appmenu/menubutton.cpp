/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "menubutton.h"

#include <QAction>
#include <QMenu>
#include <QGraphicsDropShadowEffect>

#include <Plasma/Theme>

MenuButton::MenuButton(QGraphicsWidget *parent):
    Plasma::ToolButton(parent),
    m_enterEvent(false),
    m_menu(0)
{
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(5);
    shadow->setOffset(QPointF(1, 1));
    shadow->setColor(Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
    setGraphicsEffect(shadow);
}

void MenuButton::setHovered(bool hovered)
{
    if (hovered) {
        hoverEnterEvent(0);
    } else {
        hoverLeaveEvent(0);
    }
}

QSizeF MenuButton::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
    QSizeF sh = Plasma::ToolButton::sizeHint(which, constraint);
    if (which == Qt::MinimumSize || which == Qt::PreferredSize) {
        sh.setHeight(nativeWidget()->fontMetrics().height() + bottomMargin());
    }
    return sh;
}

qreal MenuButton::bottomMargin() const
{
    qreal left, right, top, bottom;
    getContentsMargins(&left, &right, &top, &bottom);
    return bottom;
}

void MenuButton::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
    m_enterEvent = true;
    Plasma::ToolButton::hoverEnterEvent(e);
}

void MenuButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
    if (m_enterEvent) {
        m_enterEvent = false;
        Plasma::ToolButton::hoverLeaveEvent(e);
    }
}

#include "menubutton.moc"