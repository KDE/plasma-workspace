/*****************************************************************
ksmserver - the KDE session management server

Copyright 2018 David Edmundson <davidedmundson@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#pragma once

#include <KJob>
#include <QObject>
#include <QProcessEnvironment>

#include "autostart.h"

class Startup : public QObject
{
    Q_OBJECT
public:
    Startup(QObject *parent);
    void upAndRunning(const QString &msg);
    void finishStartup();
public Q_SLOTS:
    // alternatively we could drop this and have a rule that we /always/ launch everything through klauncher
    // need resolution from frameworks discussion on kdeinit
    void updateLaunchEnv(const QString &key, const QString &value);

private:
    void autoStart(int phase);
};

class SleepJob : public KJob
{
    Q_OBJECT
public:
    SleepJob();
    void start() override;
};

class KCMInitJob : public KJob
{
    Q_OBJECT
public:
    KCMInitJob();
    void start() override;
};

class KDEDInitJob : public KJob
{
    Q_OBJECT
public:
    KDEDInitJob();
    void start() override;
};

class AutoStartAppsJob : public KJob
{
    Q_OBJECT
public:
    AutoStartAppsJob(const AutoStart &autoStart, int phase);
    void start() override;

private:
    AutoStart m_autoStart;
};

/**
 * Launches a process, and waits for the process to start
 */
class StartProcessJob : public KJob
{
    Q_OBJECT
public:
    StartProcessJob(const QString &process, const QStringList &args, const QProcessEnvironment &additionalEnv = QProcessEnvironment());
    void start() override;

private:
    QProcess *m_process;
};

/**
 * Launches a process, and waits for the service to appear on the session bus
 */
class StartServiceJob : public KJob
{
    Q_OBJECT
public:
    StartServiceJob(const QString &process,
                    const QStringList &args,
                    const QString &serviceId,
                    const QProcessEnvironment &additionalEnv = QProcessEnvironment());
    void start() override;

private:
    QProcess *m_process;
    const QString m_serviceId;
    const QProcessEnvironment m_additionalEnv;
};

class RestoreSessionJob : public KJob
{
    Q_OBJECT
public:
    RestoreSessionJob();
    void start() override;

private:
};
