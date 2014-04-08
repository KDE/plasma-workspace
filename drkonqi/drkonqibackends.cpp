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
#include "drkonqibackends.h"

#include <cstdlib>
#include <cerrno>
#include <sys/types.h>
#include <signal.h>

#include <QtCore/QTimer>
#include <QtCore/QDir>

#include <KConfig>
#include <KConfigGroup>
#include <KStartupInfo>
#include <KCrash>
#include <QStandardPaths>
#include <QDebug>

#include "crashedapplication.h"
#include "debugger.h"
#include "debuggermanager.h"
#include "backtracegenerator.h"
#include "drkonqi.h"

AbstractDrKonqiBackend::~AbstractDrKonqiBackend()
{
}

bool AbstractDrKonqiBackend::init()
{
    m_crashedApplication = constructCrashedApplication();
    m_debuggerManager = constructDebuggerManager();
    return true;
}


KCrashBackend::KCrashBackend()
    : QObject(), AbstractDrKonqiBackend(), m_state(ProcessRunning)
{
}

KCrashBackend::~KCrashBackend()
{
    continueAttachedProcess();
}

bool KCrashBackend::init()
{
    AbstractDrKonqiBackend::init();

    QString startupId(DrKonqi::startupId());
    if (!startupId.isEmpty()) { // stop startup notification
        KStartupInfoId id;
        id.initId(startupId.toLocal8Bit());
        KStartupInfo::sendFinish(id);
    }

    //check whether the attached process exists and whether we have permissions to inspect it
    if (crashedApplication()->pid() <= 0) {
        qWarning() << "Invalid pid specified";
        return false;
    }

#if !defined(Q_OS_WIN32)
    if (::kill(crashedApplication()->pid(), 0) < 0) {
        switch (errno) {
        case EPERM:
            qWarning() << "DrKonqi doesn't have permissions to inspect the specified process";
            break;
        case ESRCH:
            qWarning() << "The specified process does not exist.";
            break;
        default:
            break;
        }
        return false;
    }

    //--keeprunning means: generate backtrace instantly and let the process continue execution
    if(DrKonqi::isKeepRunning()) {
        stopAttachedProcess();
        debuggerManager()->backtraceGenerator()->start();
        connect(debuggerManager(), SIGNAL(debuggerFinished()), SLOT(continueAttachedProcess()));
    } else {
        connect(debuggerManager(), SIGNAL(debuggerStarting()), SLOT(onDebuggerStarting()));
        connect(debuggerManager(), SIGNAL(debuggerFinished()), SLOT(onDebuggerFinished()));

        //stop the process to avoid high cpu usage by other threads (bug 175362).
        //if the process was started by kdeinit, we need to wait a bit for KCrash
        //to reach the alarm(0); call in kdeui/util/kcrash.cpp line 406 or else
        //if we stop it before this call, pending alarm signals will kill the
        //process when we try to continue it.
        QTimer::singleShot(2000, this, SLOT(stopAttachedProcess()));
    }
#endif

    //Handle drkonqi crashes
    s_pid = crashedApplication()->pid(); //copy pid for use by the crash handler, so that it is safer
    KCrash::setEmergencySaveFunction(emergencySaveFunction);

    return true;
}

CrashedApplication *KCrashBackend::constructCrashedApplication()
{
    CrashedApplication *a = new CrashedApplication(this);
    a->m_datetime = QDateTime::currentDateTime();
    a->m_name = DrKonqi::programName();
    a->m_version = DrKonqi::appVersion().toUtf8();
    a->m_reportAddress = BugReportAddress(DrKonqi::bugAddress().toUtf8());
    a->m_pid = DrKonqi::pid();
    a->m_signalNumber = DrKonqi::signal();
    a->m_restarted = DrKonqi::isRestarted();
    a->m_thread = DrKonqi::thread();

    //try to determine the executable that crashed
    if ( QFileInfo(QString("/proc/%1/exe").arg(a->m_pid)).exists() ) {
        //on linux, the fastest and most reliable way is to get the path from /proc
        qDebug() << "Using /proc to determine executable path";
        a->m_executable.setFile(QFile::symLinkTarget(QString("/proc/%1/exe").arg(a->m_pid)));

        if (DrKonqi::isKdeinit() ||
            a->m_executable.fileName().startsWith("python") ) {

            a->m_fakeBaseName = DrKonqi::appName();
        }
    } else {
        if ( DrKonqi::isKdeinit() ) {
            a->m_executable = QFileInfo(QStandardPaths::findExecutable("kdeinit5"));
            a->m_fakeBaseName = DrKonqi::appName();
        } else {
            QFileInfo execPath(DrKonqi::appName());
            if ( execPath.isAbsolute() ) {
                a->m_executable = execPath;
            } else if ( !DrKonqi::appPath().isEmpty() ) {
                QDir execDir(DrKonqi::appPath());
                a->m_executable = execDir.absoluteFilePath(execPath.fileName());
            } else {
                a->m_executable = QFileInfo(QStandardPaths::findExecutable(execPath.fileName()));
            }
        }
    }

    qDebug() << "Executable is:" << a->m_executable.absoluteFilePath();
    qDebug() << "Executable exists:" << a->m_executable.exists();

    return a;
}

DebuggerManager *KCrashBackend::constructDebuggerManager()
{
    QList<Debugger> internalDebuggers = Debugger::availableInternalDebuggers("KCrash");
    KConfigGroup config(KSharedConfig::openConfig(), "DrKonqi");
#ifndef Q_OS_WIN
    QString defaultDebuggerName = config.readEntry("Debugger", QString("gdb"));
#else
    QString defaultDebuggerName = config.readEntry("Debugger", QString("kdbgwin"));
#endif

    Debugger firstKnownGoodDebugger, preferredDebugger;
    foreach (const Debugger & debugger, internalDebuggers) {
        if (!firstKnownGoodDebugger.isValid() && debugger.isInstalled()) {
            firstKnownGoodDebugger = debugger;
        }
        if (debugger.codeName() == defaultDebuggerName) {
            preferredDebugger = debugger;
        }
        if (firstKnownGoodDebugger.isValid() && preferredDebugger.isValid()) {
            break;
        }
    }

    if (!preferredDebugger.isInstalled()) {
        if (firstKnownGoodDebugger.isValid()) {
            preferredDebugger = firstKnownGoodDebugger;
        } else {
            qWarning() << "Unable to find an internal debugger that can work with the KCrash backend";
        }
    }

    return new DebuggerManager(preferredDebugger, Debugger::availableExternalDebuggers("KCrash"), this);
}

void KCrashBackend::stopAttachedProcess()
{
    if (m_state == ProcessRunning) {
        qDebug() << "Sending SIGSTOP to process";
        ::kill(crashedApplication()->pid(), SIGSTOP);
        m_state = ProcessStopped;
    }
}

void KCrashBackend::continueAttachedProcess()
{
    if (m_state == ProcessStopped) {
        qDebug() << "Sending SIGCONT to process";
        ::kill(crashedApplication()->pid(), SIGCONT);
        m_state = ProcessRunning;
    }
}

void KCrashBackend::onDebuggerStarting()
{
    continueAttachedProcess();
    m_state = DebuggerRunning;
}

void KCrashBackend::onDebuggerFinished()
{
    m_state = ProcessRunning;
    stopAttachedProcess();
}

//static
qint64 KCrashBackend::s_pid = 0;

//static
void KCrashBackend::emergencySaveFunction(int signal)
{
    // In case drkonqi itself crashes, we need to get rid of the process being debugged,
    // so we kill it, no matter what its state was.
    Q_UNUSED(signal);
    ::kill(s_pid, SIGKILL);
}

#include "drkonqibackends.moc"
