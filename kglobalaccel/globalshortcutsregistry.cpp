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

#include "globalshortcutsregistry.h"
#include "component.h"
#include "globalshortcut.h"
#include "globalshortcutcontext.h"
#include <config-kglobalaccel.h>

#include <QDebug>
#include <KLocalizedString>

#include <QKeySequence>
#include <QDBusConnection>

#if HAVE_X11
#include "kglobalaccel_x11.h"
#elif defined(Q_WS_MACX)
#include "kglobalaccel_mac.h"
#elif defined(Q_WS_WIN)
#include "kglobalaccel_win.h"
#else
#include "kglobalaccel_qws.h"
#endif

GlobalShortcutsRegistry::GlobalShortcutsRegistry()
    :   QObject()
        ,_active_keys()
        ,_components()
        ,_manager(new KGlobalAccelImpl(this))
        ,_config("kglobalshortcutsrc", KConfig::SimpleConfig)
    {
    _manager->setEnabled(true);
    }


GlobalShortcutsRegistry::~GlobalShortcutsRegistry()
    {
    _manager->setEnabled(false);

    // Ungrab all keys. We don't go over GlobalShortcuts because
    // GlobalShortcutsRegistry::self() doesn't work anymore.
    Q_FOREACH (const int key, _active_keys.keys())
        {
        _manager->grabKey(key, false);
        }
    _active_keys.clear();
    }


KdeDGlobalAccel::Component *GlobalShortcutsRegistry::addComponent(KdeDGlobalAccel::Component *component)
    {
    if (_components.value(component->uniqueName()))
        {
        Q_ASSERT_X(false, "GlobalShortcutsRegistry::addComponent", "component already registered?!?!");
        return _components.value(component->uniqueName());
        }

    _components.insert(component->uniqueName(), component);
    QDBusConnection conn(QDBusConnection::sessionBus());

    conn.registerObject(
            component->dbusPath().path(),
            component,
            QDBusConnection::ExportScriptableContents);
    return component;
    }


void GlobalShortcutsRegistry::activateShortcuts()
    {
    Q_FOREACH (KdeDGlobalAccel::Component *component, _components)
        {
        component->activateShortcuts();
        }
    }


QList<KdeDGlobalAccel::Component*> GlobalShortcutsRegistry::allMainComponents() const
    {
    return _components.values();
    }


void GlobalShortcutsRegistry::clear()
    {
    Q_FOREACH(KdeDGlobalAccel::Component *component, _components)
        {
        delete component;
        }
    _components.clear();

    // The shortcuts should have deregistered themselves
    Q_ASSERT(_active_keys.isEmpty());
    }


QDBusObjectPath GlobalShortcutsRegistry::dbusPath() const
    {
    return _dbusPath;
    }


void GlobalShortcutsRegistry::deactivateShortcuts(bool temporarily)
    {
    Q_FOREACH (KdeDGlobalAccel::Component *component, _components)
        {
        component->deactivateShortcuts(temporarily);
        }
    }


GlobalShortcut *GlobalShortcutsRegistry::getActiveShortcutByKey(int key) const
    {
    return _active_keys.value(key);
    }


KdeDGlobalAccel::Component *GlobalShortcutsRegistry::getComponent(const QString &uniqueName)
    {
    return _components.value(uniqueName);
    }


GlobalShortcut *GlobalShortcutsRegistry::getShortcutByKey(int key) const
    {
    Q_FOREACH (KdeDGlobalAccel::Component *component, _components)
        {
        GlobalShortcut *rc = component->getShortcutByKey(key);
        if (rc) return rc;
        }
    return NULL;
    }


QList<GlobalShortcut*> GlobalShortcutsRegistry::getShortcutsByKey(int key) const
    {
    QList<GlobalShortcut *> rc;

    Q_FOREACH (KdeDGlobalAccel::Component *component, _components)
        {
        rc = component->getShortcutsByKey(key);
        if (!rc.isEmpty()) return rc;
        }
    return rc;
    }


bool GlobalShortcutsRegistry::isShortcutAvailable(
        int shortcut,
        const QString &componentName,
        const QString &contextName) const
    {
    Q_FOREACH (KdeDGlobalAccel::Component *component, _components)
        {
        if (!component->isShortcutAvailable(shortcut, componentName, contextName))
            return false;
        }
    return true;
    }


Q_GLOBAL_STATIC( GlobalShortcutsRegistry, _self );
GlobalShortcutsRegistry * GlobalShortcutsRegistry::self()
{
    return _self;
}


bool GlobalShortcutsRegistry::keyPressed(int keyQt)
    {
    GlobalShortcut *shortcut = getShortcutByKey(keyQt);
    if (!shortcut)
        {
        // This can happen for example with the ALT-Print shortcut of kwin.
        // ALT+PRINT is SYSREQ on my keyboard. So we grab something we think
        // is ALT+PRINT but symXToKeyQt and modXToQt make ALT+SYSREQ of it
        // when pressed (correctly). We can't match that.
        qDebug() << "Got unknown key" << QKeySequence(keyQt).toString();

        // In production mode just do nothing.
        return false;
        }
    else if (!shortcut->isActive())
        {
        qDebug() << "Got inactive key" << QKeySequence(keyQt).toString();

        // In production mode just do nothing.
        return false;
        }

    qDebug() << QKeySequence(keyQt).toString() << "=" << shortcut->uniqueName();

    QStringList data(shortcut->context()->component()->uniqueName());
    data.append(shortcut->uniqueName());
    data.append(shortcut->context()->component()->friendlyName());
    data.append(shortcut->friendlyName());

#if HAVE_X11
    // Make sure kglobalacceld has ungrabbed the keyboard after receiving the
    // keypress, otherwise actions in application that try to grab the
    // keyboard (e.g. in kwin) may fail to do so. There is still a small race
    // condition with this being out-of-process.
    KGlobalAccelImpl::syncX();
#endif

    // 1st Invoke the action
    shortcut->context()->component()->emitGlobalShortcutPressed( *shortcut );

    return true;
}


void GlobalShortcutsRegistry::loadSettings()
    {
    foreach (const QString &groupName, _config.groupList())
        {
        qDebug() << "Loading group " << groupName;

        Q_ASSERT(groupName.indexOf('\x1d')==-1);

        // loadSettings isn't designed to be called in between. Only at the
        // beginning.
        Q_ASSERT(!getComponent(groupName));

        KConfigGroup configGroup(&_config, groupName);

        // We previously stored the friendly name in a separate group. migrate
        // that
        QString friendlyName;
        KConfigGroup friendlyGroup(&configGroup, "Friendly Name");
        if (friendlyGroup.isValid())
            {
            friendlyName = friendlyGroup.readEntry("Friendly Name");
            friendlyGroup.deleteGroup();
            }
        else
            {
            friendlyName = configGroup.readEntry("_k_friendly_name");
            }

        // Create the component
        KdeDGlobalAccel::Component *component = new KdeDGlobalAccel::Component(
                groupName,
                friendlyName,
                this);

        // Now load the contexts
        Q_FOREACH(const QString& context, configGroup.groupList())
            {
            // Skip the friendly name group
            if (context=="Friendly Name") continue;

            KConfigGroup contextGroup(&configGroup, context);
            QString contextFriendlyName = contextGroup.readEntry("_k_friendly_name");
            component->createGlobalShortcutContext(context, contextFriendlyName);
            component->activateGlobalShortcutContext(context);
            component->loadSettings(contextGroup);
            }

        // Load the default context
        component->activateGlobalShortcutContext("default");
        component->loadSettings(configGroup);
        }
    }


void GlobalShortcutsRegistry::grabKeys()
    {
    activateShortcuts();
    }


bool GlobalShortcutsRegistry::registerKey(int key, GlobalShortcut *shortcut)
    {
    if (key == 0)
        {
        qDebug() << shortcut->uniqueName() << ": Key '" << QKeySequence(key).toString()
                 << "' already taken by " << _active_keys.value(key)->uniqueName() << ".";
        return false;
        }
    else if (_active_keys.value(key))
        {
        qDebug() << shortcut->uniqueName() << ": Attempt to register key 0.";
        return false;
        }

    qDebug() << "Registering key" << QKeySequence(key).toString() << "for"
             << shortcut->context()->component()->uniqueName() << ":" << shortcut->uniqueName();

    _active_keys.insert(key, shortcut);
    return _manager->grabKey(key, true);
    }


void GlobalShortcutsRegistry::setAccelManager(KGlobalAccelImpl *manager)
    {
    _manager = manager;
    }


void GlobalShortcutsRegistry::setDBusPath(const QDBusObjectPath &path)
    {
    _dbusPath = path;
    }


KdeDGlobalAccel::Component *GlobalShortcutsRegistry::takeComponent(KdeDGlobalAccel::Component *component)
    {
    QDBusConnection conn(QDBusConnection::sessionBus());
    conn.unregisterObject(component->dbusPath().path());
    return _components.take(component->uniqueName());
    }


void GlobalShortcutsRegistry::ungrabKeys()
    {
    deactivateShortcuts();
    }


bool GlobalShortcutsRegistry::unregisterKey(int key, GlobalShortcut *shortcut)
    {
    if (_active_keys.value(key)!=shortcut)
        {
        // The shortcut doesn't own the key or the key isn't grabbed
        return false;
        }

    qDebug() << "Unregistering key" << QKeySequence(key).toString() << "for"
             << shortcut->context()->component()->uniqueName() << ":" << shortcut->uniqueName();

    _manager->grabKey(key, false);
    _active_keys.take(key);
    return true;
    }


void GlobalShortcutsRegistry::writeSettings() const
    {
    Q_FOREACH(
            const KdeDGlobalAccel::Component *component,
            GlobalShortcutsRegistry::self()->allMainComponents())
        {
        KConfigGroup configGroup(&_config, component->uniqueName());
        if (component->allShortcuts().isEmpty())
            {
            configGroup.deleteGroup();
            delete component;
            }
        else
            {
            component->writeSettings(configGroup);
            }
        }

    _config.sync();
    }


#include "moc_globalshortcutsregistry.cpp"
