/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DRKONQIBACKENDS_H
#define DRKONQIBACKENDS_H

#include <QtCore/QObject>

class CrashedApplication;
class DebuggerManager;

class AbstractDrKonqiBackend
{
public:
    virtual ~AbstractDrKonqiBackend();
    virtual bool init();

    inline CrashedApplication *crashedApplication() const {
        return m_crashedApplication;
    }

    inline DebuggerManager *debuggerManager() const {
        return m_debuggerManager;
    }

protected:
    virtual CrashedApplication *constructCrashedApplication() = 0;
    virtual DebuggerManager *constructDebuggerManager() = 0;

private:
    CrashedApplication *m_crashedApplication;
    DebuggerManager *m_debuggerManager;
};

class KCrashBackend : public QObject, public AbstractDrKonqiBackend
{
    Q_OBJECT
public:
    KCrashBackend();
    virtual ~KCrashBackend();
    virtual bool init();

protected:
    virtual CrashedApplication *constructCrashedApplication();
    virtual DebuggerManager *constructDebuggerManager();

private Q_SLOTS:
    void stopAttachedProcess();
    void continueAttachedProcess();
    void onDebuggerStarting();
    void onDebuggerFinished();

private:
    static void emergencySaveFunction(int signal);
    static qint64 s_pid; //for use by the emergencySaveFunction

    enum State { ProcessRunning, ProcessStopped, DebuggerRunning };
    State m_state;
};

#endif // DRKONQIBACKENDS_H
