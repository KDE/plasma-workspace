/*
 *   Copyright 2009 by Chani Armitage <chani@kde.org>
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

#include "menu.h"

#include <QAction>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>

#include <QDebug>
#include <KMenu>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/Wallpaper>

ContextMenu::ContextMenu(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
{
    m_separator = new QAction(this);
    m_separator->setSeparator(true);
}

ContextMenu::~ContextMenu()
{
}

void ContextMenu::contextEvent(QEvent *event)
{
    QList<QAction*> actions = contextualActions();
    if (actions.isEmpty()) {
        return;
    }

    KMenu desktopMenu;
    desktopMenu.addActions(actions);
    desktopMenu.adjustSize();
    desktopMenu.exec(popupPosition(desktopMenu.size(), event));
}

QList<QAction*> ContextMenu::contextualActions()
{
    Plasma::Containment *c = containment();
    Q_ASSERT(c);
    QList<QAction*> actions;
    actions << c->contextualActions();
    if (c->wallpaper() &&
            !c->wallpaper()->contextualActions().isEmpty()) {
        actions << m_separator;
        actions << c->wallpaper()->contextualActions();
    }
    return actions;
}

#include "menu.moc"
