/*
 * Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QWindow> // for WId

#include <xcb/xcb_atom.h>

class Menu;

class MenuProxy : public QObject//, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    MenuProxy();
    ~MenuProxy() override;

protected:
    //bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) override;

private Q_SLOTS:
    void onWindowAdded(WId id);
    void onWindowRemoved(WId id);

private:
    // FIXME the get one reads "UTF8String" (reads gnome) the write thing writes "String" (writes kde)
    QByteArray getWindowPropertyString(WId id, const QByteArray &name);
    void writeWindowProperty(WId id, const QByteArray &name, const QByteArray &value);
    xcb_atom_t getAtom(const QByteArray &name);

    QHash<WId, Menu *> m_menus;

};
