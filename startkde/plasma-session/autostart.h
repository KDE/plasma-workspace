/*
    SPDX-FileCopyrightText: 2001 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QList>
#include <QStringList>

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
    QList<AutoStartItem> startList() const;

private:
    void loadAutoStartList();
    QList<AutoStartItem> m_startList;
    QStringList m_started;
    int m_phase;
    bool m_phasedone;
};
