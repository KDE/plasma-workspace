/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>

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

#ifndef SERVER_H
#define SERVER_H

#define INT32 QINT32
#include <X11/ICE/ICElib.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
extern "C" {
#include <X11/ICE/ICEmsg.h>
#include <X11/ICE/ICEproto.h>
#include <X11/ICE/ICEutil.h>
#include <X11/SM/SM.h>
#include <X11/SM/SMlib.h>
}

#include <fixx11h.h>

// needed to avoid clash with INT8 defined in X11/Xmd.h on solaris
#define QT_CLEAN_NAMESPACE 1
#include <QDBusContext>
#include <QDBusMessage>
#include <QObject>
#include <QStringList>

#include <KConfigGroup>
#include <QMap>
#include <QTime>
#include <QTimer>
#include <kmessagebox.h>
#include <kworkspace.h>

#define SESSION_PREVIOUS_LOGOUT "saved at previous logout"
#define SESSION_BY_USER "saved by user"

class KProcess;

class KSMListener;
class KSMConnection;
class KSMClient;

class OrgKdeKWinSessionInterface;

enum SMType {
    SM_ERROR,
    SM_WMCOMMAND,
    SM_WMSAVEYOURSELF,
};
struct SMData {
    SMType type;
    QStringList wmCommand;
    QString wmClientMachine;
    QString wmclass1, wmclass2;
};
typedef QMap<WId, SMData> WindowMap;

class KSMServer : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    enum class InitFlag {
        None = 0,
        OnlyLocal = 1 << 0,
        ImmediateLockScreen = 1 << 1,
        NoLockScreen = 1 << 2,
    };

    Q_DECLARE_FLAGS(InitFlags, InitFlag)
    KSMServer(InitFlags flags);
    ~KSMServer() override;

    static KSMServer *self();

    void *watchConnection(IceConn iceConn);
    void removeConnection(KSMConnection *conn);

    KSMClient *newClient(SmsConn);
    void deleteClient(KSMClient *client);

    // callbacks
    void saveYourselfDone(KSMClient *client, bool success);
    void interactRequest(KSMClient *client, int dialogType);
    void interactDone(KSMClient *client, bool cancelShutdown);
    void phase2Request(KSMClient *client);

    // error handling
    void ioError(IceConn iceConn);

    // notification
    void clientRegistered(const char *previousId);

    // public API
    void performLogout();
    void restoreSession();
    void restoreSession(const QString &sessionName);
    void startDefaultSession();
    void shutdown(KWorkSpace::ShutdownConfirm confirm, KWorkSpace::ShutdownType sdtype, KWorkSpace::ShutdownMode sdmode);

    void setupShortcuts();

Q_SIGNALS:
    void logoutCancelled();

public Q_SLOTS:

    void cleanUp();

private Q_SLOTS:
    void newConnection(int socket);
    void processData(int socket);

    void protectionTimeout();
    void timeoutQuit();

    void defaultLogout();
    void logoutWithoutConfirmation();
    void haltWithoutConfirmation();
    void rebootWithoutConfirmation();

private:
    void handlePendingInteractions();
    void completeShutdownOrCheckpoint();
    void startKilling();
    void startKillingSubSession();
    void performStandardKilling();
    void completeKilling();
    void completeKillingSubSession();
    void signalSubSessionClosed();
    void cancelShutdown(KSMClient *c);
    void killingCompleted();

    void discardSession();
    void storeSession();

    void startProtection();
    void endProtection();

    void startApplication(const QStringList &command, const QString &clientMachine = QString(), const QString &userId = QString());
    void executeCommand(const QStringList &command);

    bool defaultSession() const; // empty session
    void setupXIOErrorHandler();

    void performLegacySessionSave();
    void storeLegacySession(KConfig *config);
    void restoreLegacySession(KConfig *config);
    void restoreLegacySessionInternal(KConfigGroup *config, char sep = ',');
    QStringList windowWmCommand(WId w);
    QString windowWmClientMachine(WId w);
    WId windowWmClientLeader(WId w);
    QByteArray windowSessionId(WId w, WId leader);

    void tryRestoreNext();
    void startupDone();

    void runShutdownScripts();

    // public dcop interface

public Q_SLOTS: // public dcop interface
    void logout(int, int, int);
    bool canShutdown();
    bool isShuttingDown() const;
    QString currentSession();
    void saveCurrentSession();
    void saveCurrentSessionAs(const QString &);
    QStringList sessionList();
    void saveSubSession(const QString &name, QStringList saveAndClose, QStringList saveOnly = QStringList());
    void restoreSubSession(const QString &name);

    void openSwitchUserDialog();
    bool closeSession();

Q_SIGNALS:
    void subSessionClosed();
    void subSessionCloseCanceled();
    void subSessionOpened();
    void sessionRestored();

private:
    QList<KSMListener *> listener;
    QList<KSMClient *> clients;

    enum State {
        Idle,
        RestoringWMSession,
        Restoring,
        Shutdown,
        Checkpoint,
        Killing,
        WaitingForKNotify, // shutdown
        ClosingSubSession,
        KillingSubSession,
        RestoringSubSession,
    };
    State state;
    bool saveSession;
    int saveType;

    bool clean;
    KSMClient *clientInteracting;
    QString sessionGroup;
    QTimer protectionTimer;
    QTimer restoreTimer;
    QString xonCommand;
    // sequential startup
    int appsToStart;
    int lastAppStarted;
    QString lastIdStarted;

    QStringList excludeApps;

    WindowMap legacyWindows;

    QDBusMessage m_performLogoutCall;
    QDBusMessage m_restoreSessionCall;

    // subSession stuff
    QList<KSMClient *> clientsToKill;
    QList<KSMClient *> clientsToSave;

    OrgKdeKWinSessionInterface *m_kwinInterface;

    int sockets[2];
    friend bool readFromPipe(int pipe);
};

#endif
