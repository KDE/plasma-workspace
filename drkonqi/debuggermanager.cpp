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
#include "debuggermanager.h"

#include <KConfigGroup>

#include "debugger.h"
#include "debuggerlaunchers.h"
#include "backtracegenerator.h"

struct DebuggerManager::Private
{
    BacktraceGenerator *btGenerator;
    bool debuggerRunning;
    QList<AbstractDebuggerLauncher*> externalDebuggers;
    DBusOldInterfaceLauncher *dbusOldInterfaceLauncher;
};

DebuggerManager::DebuggerManager(const Debugger & internalDebugger,
                                 const QList<Debugger> & externalDebuggers,
                                 QObject *parent)
    : QObject(parent), d(new Private)
{
    d->debuggerRunning = false;
    d->btGenerator = new BacktraceGenerator(internalDebugger, this);
    connect(d->btGenerator, SIGNAL(starting()), SLOT(onDebuggerStarting()));
    connect(d->btGenerator, SIGNAL(done()), SLOT(onDebuggerFinished()));
    connect(d->btGenerator, SIGNAL(someError()), SLOT(onDebuggerFinished()));
    connect(d->btGenerator, SIGNAL(failedToStart()), SLOT(onDebuggerFinished()));

    foreach(const Debugger & debugger, externalDebuggers) {
        if (debugger.isInstalled()) {
            AbstractDebuggerLauncher *l = new DefaultDebuggerLauncher(debugger, this); //FIXME
            d->externalDebuggers.append(l);
            connect(l, SIGNAL(starting()), SLOT(onDebuggerStarting()));
            connect(l, SIGNAL(finished()), SLOT(onDebuggerFinished()));
            connect(l, SIGNAL(invalidated()), SLOT(onDebuggerInvalidated()));
        }
    }

    //setup kdevelop compatibility
    d->dbusOldInterfaceLauncher = new DBusOldInterfaceLauncher(this);
    connect(d->dbusOldInterfaceLauncher, SIGNAL(starting()), SLOT(onDebuggerStarting()));
    connect(d->dbusOldInterfaceLauncher, SIGNAL(available()), SLOT(onDBusOldInterfaceDebuggerAvailable()));
}

DebuggerManager::~DebuggerManager()
{
    if (d->btGenerator->state() == BacktraceGenerator::Loading) {
        //if the debugger is running, kill it and continue the process.
        delete d->btGenerator;
        onDebuggerFinished();
    }

    delete d;
}

bool DebuggerManager::debuggerIsRunning() const
{
    return d->debuggerRunning;
}

bool DebuggerManager::showExternalDebuggers() const
{
    KConfigGroup config(KSharedConfig::openConfig(), "DrKonqi");
    bool showDebugger = config.readEntry("ShowDebugButton", false);

    // TODO: remove all these compatibility code when KDE SC 4.11
    // is considered as totally outdated
    //
    //for compatibility with drkonqi 1.0, if "ShowDebugButton" is not specified in the config
    //and the old "ConfigName" key exists and is set to "developer", we show the debug button.
    if (!config.hasKey("ShowDebugButton") &&
        config.readEntry("ConfigName") == "developer") {
        showDebugger = true;
        // migrate and remove the long deprecated entry
        config.writeEntry("ShowDebugButton", true);
        config.deleteEntry("ConfigName");
    }

    return showDebugger;
}

QList<AbstractDebuggerLauncher*> DebuggerManager::availableExternalDebuggers() const
{
    return d->externalDebuggers;
}

BacktraceGenerator* DebuggerManager::backtraceGenerator() const
{
    return d->btGenerator;
}

void DebuggerManager::onDebuggerStarting()
{
    d->debuggerRunning = true;
    emit debuggerStarting();
    emit debuggerRunning(true);
}

void DebuggerManager::onDebuggerFinished()
{
    d->debuggerRunning = false;
    emit debuggerFinished();
    emit debuggerRunning(false);
}

void DebuggerManager::onDebuggerInvalidated()
{
    AbstractDebuggerLauncher *launcher = qobject_cast<AbstractDebuggerLauncher*>(sender());
    Q_ASSERT(launcher);
    int index = d->externalDebuggers.indexOf(launcher);
    Q_ASSERT(index >= 0);
    d->externalDebuggers.removeAt(index);
    emit externalDebuggerRemoved(launcher);
}

void DebuggerManager::onDBusOldInterfaceDebuggerAvailable()
{
    d->externalDebuggers.append(d->dbusOldInterfaceLauncher);
    emit externalDebuggerAdded(d->dbusOldInterfaceLauncher);
}

#include "debuggermanager.moc"
