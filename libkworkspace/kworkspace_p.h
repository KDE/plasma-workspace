/* This file is part of the KDE libraries
    Copyright (C) 2007 Lubos Lunak <l.lunak@kde.org>

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

#ifndef KWORKSPACE_P_H
#define KWORKSPACE_P_H

#include <QObject>
#include "kworkspace.h"
#include "config-libkworkspace.h"

#if HAVE_X11
#include <X11/SM/SMlib.h>
#endif

class QSocketNotifier;

namespace KWorkSpace
{

// A class that creates another connection to ksmserver and handles it properly.
class KRequestShutdownHelper
    : public QObject
    {
    Q_OBJECT
    public:
        KRequestShutdownHelper();
        ~KRequestShutdownHelper() override;
        bool requestShutdown( ShutdownConfirm confirm );
    private Q_SLOTS:
        void processData();
    private:
#if HAVE_X11
        SmcConn connection() const { return conn; }
        SmcConn conn;
#endif
        QSocketNotifier* notifier;
    };
 
}


#endif
