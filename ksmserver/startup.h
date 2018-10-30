#pragma once

#include <QObject>
#include <KJob>

#include "autostart.h"

class KProcess;
class KSMServer;
class KCompositeJob;

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
public Q_SLOTS:
    void done();
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

