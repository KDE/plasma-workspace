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

#include "globalshortcut.h"

#warning Remove private header kglobalshortcutinfo_p.once this kglobalaccel daemon is merged with the kglobalaccel framework
#include "kglobalshortcutinfo_p.h"

#include "component.h"
#include "globalshortcutcontext.h"
#include "globalshortcutsregistry.h"


#include <QDebug>

#include <QKeySequence>


GlobalShortcut::GlobalShortcut()
        :   _isPresent(false)
            ,_isRegistered(false)
            ,_isFresh(true)
            ,_context(NULL)
            ,_uniqueName()
            ,_friendlyName()
            ,_keys()
            ,_defaultKeys()
    {}


GlobalShortcut::GlobalShortcut(
        const QString &uniqueName,
        const QString &friendlyName,
        GlobalShortcutContext *context)
            :   _isPresent(false)
                ,_isRegistered(false)
                ,_isFresh(true)
                ,_context(context)
                ,_uniqueName(uniqueName)
                ,_friendlyName(friendlyName)
                ,_keys()
                ,_defaultKeys()
    {
    context->addShortcut(this);
    }


GlobalShortcut::~GlobalShortcut()
    {
    setInactive();
    }


GlobalShortcut::operator KGlobalShortcutInfo () const
    {
    KGlobalShortcutInfo info;
    info.d->uniqueName = _uniqueName;
    info.d->friendlyName = _friendlyName;
    info.d->contextUniqueName = context()->uniqueName();
    info.d->contextFriendlyName = context()->friendlyName();
    info.d->componentUniqueName = context()->component()->uniqueName();
    info.d->componentFriendlyName = context()->component()->friendlyName();
    Q_FOREACH (int key, _keys)
        {
        info.d->keys.append(QKeySequence(key));
        }
    Q_FOREACH (int key, _defaultKeys)
        {
        info.d->defaultKeys.append(QKeySequence(key));
        }
    return info;
    }


bool GlobalShortcut::isActive() const
    {
    return _isRegistered;
    }


bool GlobalShortcut::isFresh() const
    {
    return _isFresh;
    }


bool GlobalShortcut::isPresent() const
    {
    return _isPresent;
    }


bool GlobalShortcut::isSessionShortcut() const
    {
    return uniqueName().startsWith(QLatin1String("_k_session:"));
    }


void GlobalShortcut::setIsFresh(bool value)
    {
    _isFresh = value;
    }


void GlobalShortcut::setIsPresent(bool value)
    {
    // (de)activate depending on old/new value
    _isPresent = value;
    value
        ? setActive()
        : setInactive();
    }


GlobalShortcutContext *GlobalShortcut::context()
    {
    return _context;
    }


GlobalShortcutContext const *GlobalShortcut::context() const
    {
    return _context;
    }


QString GlobalShortcut::uniqueName() const
    {
    return _uniqueName;
    }


void GlobalShortcut::unRegister()
    {
    return _context->component()->unregisterShortcut(uniqueName());
    }


QString GlobalShortcut::friendlyName() const
    {
    return _friendlyName;
    }


void GlobalShortcut::setFriendlyName(const QString &name)
    {
    _friendlyName = name;
    }


QList<int> GlobalShortcut::keys() const
    {
    return _keys;
    }


void GlobalShortcut::setKeys(const QList<int> newKeys)
    {
    bool active = _isRegistered;
    if (active)
        {
        setInactive();
        }

    _keys = QList<int>();

    Q_FOREACH(int key, newKeys)
        {
        if (key!=0 && !GlobalShortcutsRegistry::self()->getShortcutByKey(key))
            {
            _keys.append(key);
            }
        else
            {
            qDebug() << _uniqueName << "skipping because key" << QKeySequence(key).toString() << "is already taken";
            _keys.append(0);
            }
        }

    if (active)
        {
        setActive();
        }
    }


QList<int> GlobalShortcut::defaultKeys() const
    {
    return _defaultKeys;
    }


void GlobalShortcut::setDefaultKeys(const QList<int> newKeys)
    {
    _defaultKeys = newKeys;
    }


void GlobalShortcut::setActive()
    {
    if (!_isPresent || _isRegistered)
        {
        // The corresponding application is not present or the keys are
        // already grabbed
        return;
        }

    Q_FOREACH( int key, _keys)
        {
        if (key != 0 && !GlobalShortcutsRegistry::self()->registerKey(key, this))
            {
            qDebug() << uniqueName() << ": Failed to register " << QKeySequence(key).toString();
            }
        }

    _isRegistered = true;
    }


void GlobalShortcut::setInactive()
    {
    if (!_isRegistered)
        {
        // The keys are not grabbed currently
        return;
        }

    Q_FOREACH( int key, _keys)
        {
        if (key != 0 && !GlobalShortcutsRegistry::self()->unregisterKey(key, this))
            {
            qDebug() << uniqueName() << ": Failed to unregister " << QKeySequence(key).toString();
            }
        }

    _isRegistered = false;
    }

