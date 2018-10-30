#pragma once

#include <QObject>
#include <QDBusInterface>
#include "autostart.h"

class KProcess;
class KSMServer;

class Startup : public QObject
{
public:
    Startup(KSMServer *parent);
    void upAndRunning( const QString& msg );
private Q_SLOTS:

    void runUserAutostart();
    bool migrateKDE4Autostart(const QString &autostartFolder);

    void autoStart0();
    void autoStart1();
    void autoStart2();
    void autoStart0Done();
    void autoStart1Done();
    void autoStart2Done();
    void kcmPhase1Done();
    void kcmPhase2Done();
    // ksplash interface
    void finishStartup();
    void slotAutoStart();
    void secondKDEDPhaseLoaded();
    void kcmPhase1Timeout();
    void kcmPhase2Timeout();

private:
    void autoStart(int phase);

private:
    AutoStart m_autoStart;
    KSMServer *ksmserver = nullptr;
    enum State
    {
        Waiting, AutoStart0, KcmInitPhase1, AutoStart1, FinishingStartup, // startup
    };
    State state;

    bool waitAutoStart2 = true;
    bool waitKcmInit2  = true;
    QDBusInterface* kcminitSignals = nullptr;

};
