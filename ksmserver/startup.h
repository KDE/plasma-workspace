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

#include <QObject>
#include <KJob>

#include "autostart.h"

class KSMServer;

class Startup : public QObject
{
Q_OBJECT
public:
    Startup(KSMServer *parent);
    void upAndRunning( const QString& msg );
    void finishStartup();
private:
    void autoStart(int phase);

private:
    KSMServer *ksmserver = nullptr;
};

class KCMInitJob: public KJob
{
Q_OBJECT
public:
    KCMInitJob(int phase);
    void start() override;
private:
    int m_phase;
};

class KDEDInitJob: public KJob
{
Q_OBJECT
public:
    KDEDInitJob();
    void start() override;
};

class AutoStartAppsJob: public KJob
{
Q_OBJECT
public:
    AutoStartAppsJob(int phase);
    void start() override;
private:
    AutoStart m_autoStart;
};

class RestoreSessionJob: public KJob
{
Q_OBJECT
public:
    RestoreSessionJob(KSMServer *ksmserver);
    void start() override;
private:
    KSMServer *m_ksmserver;
};

