/* This file is part of the KDE libraries
    Copyright (C) 2001,2002 Ellis Whitehead <ellis@kde.org>

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

#ifndef _KGLOBALACCEL_WIN_H
#define _KGLOBALACCEL_WIN_H

#include <QWidget>

class GlobalShortcutsRegistry;
/**
 * @internal
 *
 * The KGlobalAccel private class handles grabbing of global keys,
 * and notification of when these keys are pressed.
 */
class KGlobalAccelImpl : public QWidget
{
    Q_OBJECT

public:
    KGlobalAccelImpl(GlobalShortcutsRegistry* owner);

public:
    /**
    * This function registers or unregisters a certain key for global capture,
    * depending on \b grab.
    *
    * Before destruction, every grabbed key will be released, so this
    * object does not need to do any tracking.
    *
    * \param key the Qt keycode to grab or release.
    * \param grab true to grab they key, false to release the key.
    *
    * \return true if successful, otherwise false.
    */
    bool grabKey(int key, bool grab);

    /// Enable/disable all shortcuts. There will not be any grabbed shortcuts at this point.
    void setEnabled(bool);

private:
    bool winEvent(MSG * message, long * result);

    GlobalShortcutsRegistry* m_owner;
    bool m_enabled;
};

#endif // _KGLOBALACCEL_WIN_H
