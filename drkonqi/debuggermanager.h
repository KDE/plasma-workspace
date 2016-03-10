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
#ifndef DEBUGGERMANAGER_H
#define DEBUGGERMANAGER_H

#include <QtCore/QObject>

class BacktraceGenerator;
class Debugger;
class AbstractDebuggerLauncher;

class DebuggerManager : public QObject
{
    Q_OBJECT
public:
    DebuggerManager(const Debugger & internalDebugger,
                    const QList<Debugger> & externalDebuggers,
                    QObject *parent = 0);
    ~DebuggerManager() override;

    bool debuggerIsRunning() const;
    bool showExternalDebuggers() const;
    QList<AbstractDebuggerLauncher*> availableExternalDebuggers() const;
    BacktraceGenerator *backtraceGenerator() const;

Q_SIGNALS:
    void debuggerStarting();
    void debuggerFinished();
    void debuggerRunning(bool running);
    void externalDebuggerAdded(AbstractDebuggerLauncher *launcher);
    void externalDebuggerRemoved(AbstractDebuggerLauncher *launcher);

private Q_SLOTS:
    void onDebuggerStarting();
    void onDebuggerFinished();
    void onDebuggerInvalidated();
    void onDBusOldInterfaceDebuggerAvailable();

private:
    struct Private;
    Private *const d;
};

#endif // DEBUGGERMANAGER_H
