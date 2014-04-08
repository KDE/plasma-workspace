#ifndef GLOBALSHORTCUTCONTEXT_H
#define GLOBALSHORTCUTCONTEXT_H
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

#include "kglobalshortcutinfo.h"

#include <QtCore/QString>
#include <QtCore/QHash>

namespace KdeDGlobalAccel {
    class Component;
}

class GlobalShortcut;

/**
 * @author Michael Jansen <kde@michael-jansen.biz>
 */
class GlobalShortcutContext
    {
public:

    /**
     * Default constructor
     */
    GlobalShortcutContext(
            const QString &uniqueName,
            const QString &friendlyName,
            KdeDGlobalAccel::Component *component );

    /**
     * Destructor
     */
    virtual ~GlobalShortcutContext();

    //! Adds @p shortcut to the context
    void addShortcut(GlobalShortcut *shortcut);

    //! Return KGlobalShortcutInfos for all shortcuts
    QList<KGlobalShortcutInfo> allShortcutInfos() const;

    /**
     * Get the name for the context
     */
    QString uniqueName() const;
    QString friendlyName() const;

    KdeDGlobalAccel::Component *component();
    KdeDGlobalAccel::Component const *component() const;

    //! Get shortcut for @p key or NULL
    GlobalShortcut *getShortcutByKey(int key) const;

    //! Remove @p shortcut from the context. The shortcut is not deleted.
    GlobalShortcut *takeShortcut(GlobalShortcut *shortcut);

private:

    friend class KdeDGlobalAccel::Component;

    //! The unique name for this context
    QString _uniqueName;

    //! The unique name for this context
    QString _friendlyName;

    //! The component the context belongs too
    KdeDGlobalAccel::Component *_component;

    //! The actions associated with this context
    QHash<QString, GlobalShortcut*> _actions;
};


#endif /* #ifndef GLOBALSHORTCUTCONTEXT_H */
