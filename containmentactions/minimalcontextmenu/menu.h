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

#ifndef CONTEXTMENU_HEADER
#define CONTEXTMENU_HEADER

#include <QButtonGroup>
#include <plasma/containmentactions.h>

class ContextMenu : public Plasma::ContainmentActions
{
    Q_OBJECT
public:
    ContextMenu(QObject* parent, const QVariantList& args);
    ~ContextMenu();

    void contextEvent(QEvent *event);
    QList<QAction*> contextualActions();

private:
    QAction *m_separator;

};

K_EXPORT_PLASMA_CONTAINMENTACTIONS(minimalcontextmenu, ContextMenu)

#endif
