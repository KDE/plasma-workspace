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

#include "kglobalaccel_win.h"

#include "kkeyserver_win.h"

#include <QWidgetList>
#ifdef Q_WS_WIN

#include "kglobalaccel.h"
#include "globalshortcutsregistry.h"

#include <kapplication.h>
#include <QDebug>

#include <windows.h>

KGlobalAccelImpl::KGlobalAccelImpl(GlobalShortcutsRegistry* owner)
    : m_owner(owner), m_enabled(false)
{
}

bool KGlobalAccelImpl::grabKey( int keyQt, bool grab )
{
    if( !keyQt ) {
        kWarning(125) << "Tried to grab key with null code.";
        return false;
    }

    uint keyCodeW;
    uint keyModW;
    KKeyServer::keyQtToCodeWin(keyQt, &keyCodeW);
    KKeyServer::keyQtToModWin(keyQt, &keyModW);

    ATOM id = GlobalAddAtom(MAKEINTATOM(keyQt));
    bool b;
    if (grab) {
        b = RegisterHotKey(winId(), id, keyModW, keyCodeW);
    } else {
        b = UnregisterHotKey(winId(), id);
    }

    return b;
}

void KGlobalAccelImpl::setEnabled( bool enable )
{
    m_enabled = enable;
}

bool KGlobalAccelImpl::winEvent( MSG * message, long * result )
{
    if (message->message == WM_HOTKEY) {
        uint keyCodeW = HIWORD(message->lParam);
        uint keyModW = LOWORD(message->lParam);

        int keyCodeQt, keyModQt;
        KKeyServer::codeWinToKeyQt(keyCodeW, &keyCodeQt);
        KKeyServer::modWinToKeyQt(keyModW, &keyModQt);

        return m_owner->keyPressed(keyCodeQt | keyModQt);
    }
    return false;
}

#include "kglobalaccel_win.moc"

#endif // Q_WS_WIN
