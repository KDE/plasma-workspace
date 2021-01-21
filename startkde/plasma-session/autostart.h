/*
   This file is part of the KDE libraries
   Copyright (c) 2001 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _AUTOSTART_H_
#define _AUTOSTART_H_

#include <QStringList>
#include <QVector>

class AutoStartItem
{
public:
    QString name;
    QString service;
    QString startAfter;
    int phase;
};

class AutoStart
{
public:
    AutoStart();
    ~AutoStart();

    QString startService();
    void setPhase(int phase);
    void setPhaseDone();
    int phase() const
    {
        return m_phase;
    }
    bool phaseDone() const
    {
        return m_phasedone;
    }
    QVector<AutoStartItem> startList() const;

private:
    void loadAutoStartList();
    QVector<AutoStartItem> m_startList;
    QStringList m_started;
    int m_phase;
    bool m_phasedone;
};

#endif
