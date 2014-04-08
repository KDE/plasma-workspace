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

#include "kglobalaccel_qws.h"

#include <config.h>

#include <QWidgetList>
#ifdef Q_WS_QWS

#include "kglobalaccel.h"

#include <kapplication.h>
#include <QDebug>

KGlobalAccelImpl::KGlobalAccelImpl(GlobalShortcutsRegistry* owner)
	: m_owner(owner)
{
}

bool KGlobalAccelImpl::grabKey( int keyQt, bool grab )
{
	if( !keyQt ) {
		kWarning(125) << "Tried to grab key with null code.";
		return false;
	}

	// TODO ...

	return false;
}

void KGlobalAccelImpl::setEnabled( bool enable )
{
#if 0
	if ( enable )
		kapp->installWinEventFilter( this );
	else
		kapp->removeWinEventFilter( this );
#endif
}

#include "kglobalaccel_qws.moc"

#endif // Q_WS_QWS
