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
#ifndef DRKONQI_H
#define DRKONQI_H

#include <QString>

class QWidget;

class SystemInformation;
class DebuggerManager;
class CrashedApplication;
class AbstractDrKonqiBackend;

class DrKonqi
{
public:
    static bool init();
    static void cleanup();

    static SystemInformation *systemInformation();
    static DebuggerManager *debuggerManager();
    static CrashedApplication *crashedApplication();

    static void saveReport(const QString & reportText, QWidget *parent = 0);

    static void setSignal(int signal);
    static void setAppName(const QString &appName);
    static void setAppPath(const QString &appPath);
    static void setAppVersion(const QString &appVersion);
    static void setBugAddress(const QString &bugAddress);
    static void setProgramName(const QString &programName);
    static void setPid(int pid);
    static void setStartupId(const QString &startupId);
    static void setKdeinit(bool kdeinit);
    static void setSafer(bool safer);
    static void setRestarted(bool restarted);
    static void setKeepRunning(bool keepRunning);
    static void setThread(int thread);

    static int signal();
    static const QString &appName();
    static const QString &appPath();
    static const QString &appVersion();
    static const QString &bugAddress();
    static const QString &programName();
    static int pid();
    static const QString &startupId();
    static bool isKdeinit();
    static bool isSafer();
    static bool isRestarted();
    static bool isKeepRunning();
    static int thread();

private:
    DrKonqi();
    ~DrKonqi();
    static DrKonqi *instance();

    SystemInformation *m_systemInformation;
    AbstractDrKonqiBackend *m_backend;

    int m_signal;
    QString m_appName;
    QString m_appPath;
    QString m_appVersion;
    QString m_bugAddress;
    QString m_programName;
    int m_pid;
    QString m_startupId;
    bool m_kdeinit;
    bool m_safer;
    bool m_restarted;
    bool m_keepRunning;
    int m_thread;
};

#endif
