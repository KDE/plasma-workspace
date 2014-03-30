/* This file is part of the KDE project
   Copyright (C) 2001 Lubos Lunak <l.lunak@kde.org>

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

#ifndef __startup_h__
#define __startup_h__

#include <sys/types.h>

#include <QWidget>
#include <QPixmap>

#include <QTimer>
#include <QMap>
#include <kstartupinfo.h>

class KSelectionWatcher;

class StartupId
    : public QWidget
    {
    Q_OBJECT
    public:
        explicit StartupId( QWidget* parent = 0, const char* name = 0 );
        virtual ~StartupId();
        void configure();
    protected:
        virtual bool x11Event( XEvent* e );
        void start_startupid( const QString& icon );
        void stop_startupid();
    protected Q_SLOTS:
        void update_startupid();
        void gotNewStartup( const KStartupInfoId& id, const KStartupInfoData& data );
        void gotStartupChange( const KStartupInfoId& id, const KStartupInfoData& data );
        void gotRemoveStartup( const KStartupInfoId& id );
        void finishKDEStartup();
        void newOwner();
        void lostOwner();
    protected:
        KStartupInfo startup_info;
        WId startup_window;
        QTimer update_timer;
        QMap< KStartupInfoId, QString > startups; // QString == pixmap
        KStartupInfoId current_startup;
        bool blinking;
        bool bouncing;
        unsigned int color_index;
        unsigned int frame;
        enum { NUM_BLINKING_PIXMAPS = 5 };
        QPixmap pixmaps[ NUM_BLINKING_PIXMAPS ];
        KSelectionWatcher* selection_watcher;
        bool active_selection;
    };

#endif
