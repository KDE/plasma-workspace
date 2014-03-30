/******************************************************************************
*   Copyright 2013 Sebastian Kügler <sebas@kde.org>                           *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

#ifndef PUMPJOB_H
#define PUMPJOB_H

#include <KJob>

#include <KIO/Job>

class PumpJobPrivate;

class PumpJob : public KIO::Job
{
    Q_OBJECT

    public:
        PumpJob(QObject* parent = 0, int interval = 0);
        virtual ~PumpJob();

        virtual void start();
        virtual bool doKill();
        virtual bool doSuspend();
        virtual bool doResume();

        virtual bool isSuspended() const;

        void init();
    Q_SIGNALS:
        void suspended(KJob *job);
        void resumed(KJob *job);

public Q_SLOTS:
        void timeout();

    private:
        PumpJobPrivate* d;
};

#endif
