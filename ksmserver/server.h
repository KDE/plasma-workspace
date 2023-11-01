/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#define INT32 QINT32
#include <X11/ICE/ICElib.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
extern "C" {
#include <X11/SM/SM.h>
#include <X11/SM/SMlib.h>
}

#include <fixx11h.h>

#include <QDBusContext>
#include <QDBusMessage>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QWindow>

#include <KConfigGroup>
#include <kworkspace.h>

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
using WindowMap = QMap<WId, SMData>;

constexpr QLatin1String SESSION_PREFIX("Session: ");
constexpr QLatin1String SUBSESSION_PREFIX("SubSession: ");
constexpr QLatin1String SESSION_PREVIOUS_LOGOUT("saved at previous logout");
constexpr QLatin1String SESSION_BY_USER("saved by user");

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
    explicit KSMServer(InitFlags flags);
    ~KSMServer() override;
    Q_DISABLE_COPY_MOVE(KSMServer);

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
    void setRestoreSession(const QString &sessionName);
    void startDefaultSession();

Q_SIGNALS:
    void logoutFinished(bool sessionClosed);

public Q_SLOTS:

    void cleanUp();

private Q_SLOTS:
    void newConnection(int socket);
    void processData(int socket);

    void protectionTimeout();
    void timeoutQuit();

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

    void tryRestore();
    void startupDone();

    void runShutdownScripts();

public Q_SLOTS: // public dbus interface
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
    QString xonCommand;
    // sequential startup
    int appsToStart;

    QStringList excludeApps;

    WindowMap legacyWindows;

    QDBusMessage m_restoreSessionCall;

    // subSession stuff
    QList<KSMClient *> clientsToKill;
    QList<KSMClient *> clientsToSave;

    OrgKdeKWinSessionInterface *m_kwinInterface;

    int sockets[2];
    friend bool readFromPipe(int pipe);
};
