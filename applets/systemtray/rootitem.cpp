/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
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

#include "rootitem.h"

#include <QDebug>




RootItem::RootItem(QQuickItem *parent)
    : QQuickItem(parent)
{
 
}

RootItem::~RootItem()
{
}

void Plasma::setApplet(Plasma::Applet *applet)
{
    if (m_applet == applet) {
        return;
    }

    m_applet = applet;
    emit compactRepresentationItemChanged();
    emit fullRepresentationItemChanged();
}

Plasma::Applet *Plasma::applet()
{
    return m_applet;
}


QQuickItem *RootItem::compactRepresentationItem()
{
    if (!m_appletInterface) {
        return;
    }

    return m_appletInterface->property("compactRepresentationItem");
}

QQuickItem *RootItem::fullRepresentationItem()
{
    if (!m_appletInterface) {
        return;
    }

    return m_appletInterface->property("fullRepresentationItem");
}

bool RootItem::isExpanded() const
{
    return true;
}

QObject *RootItem::rootItem()
{
    if (!m_appletInterface) {
        return;
    }

    return m_appletInterface->property("rootItem");
}

#include "moc_rootitem.cpp"

