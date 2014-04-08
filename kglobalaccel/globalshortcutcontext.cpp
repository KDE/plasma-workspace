/* Copyright (C) 2008 Michael Jansen <kde@michael-jansen.biz>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "globalshortcutcontext.h"

#include "globalshortcut.h"

#include <QDebug>

GlobalShortcutContext::GlobalShortcutContext(
        const QString &uniqueName,
        const QString &friendlyName,
        KdeDGlobalAccel::Component *component)

        :   _uniqueName(uniqueName),
            _friendlyName(friendlyName),
            _component(component),
            _actions()
    {}


GlobalShortcutContext::~GlobalShortcutContext()
    {
    qDeleteAll(_actions); _actions.clear();
    }


void GlobalShortcutContext::addShortcut(GlobalShortcut *shortcut)
    {
    _actions.insert(shortcut->uniqueName(), shortcut);
    }


QList<KGlobalShortcutInfo> GlobalShortcutContext::allShortcutInfos() const
    {
    QList<KGlobalShortcutInfo> rc;
    Q_FOREACH (GlobalShortcut *shortcut, _actions)
        {
        rc.append(static_cast<KGlobalShortcutInfo>(*shortcut));
        }
    return rc;
    }


KdeDGlobalAccel::Component const *GlobalShortcutContext::component() const
    {
    return _component;
    }


KdeDGlobalAccel::Component *GlobalShortcutContext::component()
    {
    return _component;
    }


QString GlobalShortcutContext::friendlyName() const
    {
    return _friendlyName;
    }


GlobalShortcut *GlobalShortcutContext::getShortcutByKey(int key) const
    {
    // Qt triggers both shortcuts that include Shift+Backtab and Shift+Tab
    // when user presses Shift+Tab. Do the same here.
    int keySym = key & ~Qt::KeyboardModifierMask;
    int keyMod = key & Qt::KeyboardModifierMask;
    if ((keyMod & Qt::SHIFT) && (keySym == Qt::Key_Backtab ||
        keySym == Qt::Key_Tab))
        {
        Q_FOREACH(GlobalShortcut *sc, _actions)
            {
            if (sc->keys().contains(keyMod | Qt::Key_Tab) ||
                sc->keys().contains(keyMod | Qt::Key_Backtab))
                return sc;
            }
        }
    else
        {
        Q_FOREACH(GlobalShortcut *sc, _actions)
            {
            if (sc->keys().contains(key)) return sc;
            }
        }
    return NULL;
    }


GlobalShortcut *GlobalShortcutContext::takeShortcut(GlobalShortcut *shortcut)
    {
    // Try to take the shortcut. Result could be null if the shortcut doesn't
    // belong to this component.
    return _actions.take(shortcut->uniqueName());
    }


QString GlobalShortcutContext::uniqueName() const
    {
    return _uniqueName;
    }
