#ifndef GLOBALSHORTCUTSREGISTRY_H
#define GLOBALSHORTCUTSREGISTRY_H
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

#include "ksharedconfig.h"

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtDBus/QDBusObjectPath>


class GlobalShortcut;
class KGlobalAccelImpl;

namespace KdeDGlobalAccel
    {
    class Component;
    }

/**
 * Global Shortcut Registry.
 *
 * Shortcuts are registered by component. A component is for example kmail or
 * amarok.
 *
 * A component can have contexts. Currently on plasma is planned to support
 * that feature. A context enables plasma to keep track of global shortcut
 * settings when switching containments.
 *
 * A shortcut (WIN+x) can be registered by one component only. The component
 * is allowed to register it more than once in different contexts.
 *
 * @author Michael Jansen <kde@michael-jansen.biz>
 */
class GlobalShortcutsRegistry : public QObject
    {
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.kde.KdedGlobalAccel.GlobalShortcutsRegistry")

public:
    /**
     * Use GlobalShortcutsRegistry::self()
     *
     * @internal
     */
    GlobalShortcutsRegistry();
    ~GlobalShortcutsRegistry();

    /**
     * Activate all shortcuts having their application present.
     */
    void activateShortcuts();

    /**
     * Return a list of all main components
     */
    QList<KdeDGlobalAccel::Component *> allMainComponents() const;

    /**
     * Return the root dbus path for the registry.
     */
    QDBusObjectPath dbusPath() const;

    /**
     * Deactivate all currently active shortcuts.
     */
    void deactivateShortcuts(bool temporarily=false);


    /**
     * Get the shortcut corresponding to key. Only active shortcut are
     * considered.
     */
    GlobalShortcut *getActiveShortcutByKey(int key) const;

    /**
     */
    KdeDGlobalAccel::Component *getComponent(const QString &uniqueName);

    /**
     * Get the shortcut corresponding to key. Active and inactive shortcuts
     * are considered. But if the matching application uses contexts only one
     * shortcut is returned.
     *
     * @see getShortcutsByKey(int key)
     */
    GlobalShortcut *getShortcutByKey(int key) const;

    /**
     * Get the shortcuts corresponding to key. Active and inactive shortcuts
     * are considered.
     *
     * @see getShortcutsByKey(int key)
     */
    QList<GlobalShortcut*> getShortcutsByKey(int key) const;

    /**
     * Checks if @p shortcut is available for @p component.
     *
     * It is available if not used by another component in any context or used
     * by @p component only in not active contexts.
     */
    bool isShortcutAvailable(
            int shortcut,
            const QString &component,
            const QString &context) const;

    static GlobalShortcutsRegistry *self();

    bool registerKey(int key, GlobalShortcut *shortcut);

    void setAccelManager(KGlobalAccelImpl *manager);

    void setDBusPath(const QDBusObjectPath &path);

    bool unregisterKey(int key, GlobalShortcut *shortcut);

public Q_SLOTS:

    void clear();

    void loadSettings();

    void writeSettings() const;

    // Grab the keys
    void grabKeys();

    // Ungrab the keys
    void ungrabKeys();

private:

    friend class KdeDGlobalAccel::Component;
    friend class KGlobalAccelImpl;

    KdeDGlobalAccel::Component *addComponent(KdeDGlobalAccel::Component *component);
    KdeDGlobalAccel::Component *takeComponent(KdeDGlobalAccel::Component *component);

    //called by the implementation to inform us about key presses
    //returns true if the key was handled
    bool keyPressed(int keyQt);

    QHash<int, GlobalShortcut*> _active_keys;
    QHash<QString, KdeDGlobalAccel::Component *> _components;

    KGlobalAccelImpl *_manager;

    mutable KConfig _config;

    QDBusObjectPath _dbusPath;
    };


#endif /* #ifndef GLOBALSHORTCUTSREGISTRY_H */
