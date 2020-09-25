/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>
Copyright 2005 Lubos Lunak <l.lunak@kde.org>

relatively small extensions by Oswald Buddenhagen <ob6@inf.tu-dresden.de>

some code taken from the dcopserver (part of the KDE libraries), which is
Copyright 1999 Matthias Ettrich <ettrich@kde.org>
Copyright 1999 Preston Brown <pbrown@kde.org>

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

#include "server.h"
#include "global.h"
#include "client.h"
#include "ksmserver_debug.h"
#include "ksmserverinterfaceadaptor.h"
#include "klocalizedstring.h"
#include "kglobalaccel.h"

#include <config-workspace.h>
#include <config-unix.h> // HAVE_LIMITS_H
#include <config-ksmserver.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <QFile>
#include <QPushButton>
#include <QRegularExpression>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QDebug>
#include <QAction>
#include <QApplication>

#include <kactioncollection.h>
#include <kauthorized.h>
#include <kconfig.h>
#include <KSharedConfig>
#include <kdesktopfile.h>
#include <QTemporaryFile>
#include <kconfiggroup.h>
#include <kprocess.h>
#include <kshell.h>

#include <KIO/CommandLauncherJob>

#include <KScreenLocker/KsldApp>

#include <QX11Info>
#include <krandom.h>
#include <klauncher_interface.h>
#include <startup_interface.h>
#include <qstandardpaths.h>

#include "kscreenlocker_interface.h"
#include "kwinsession_interface.h"

#include <updatelaunchenvjob.h>

KSMServer* the_server = nullptr;

KSMServer* KSMServer::self()
{
    return the_server;
}

/*! Utility function to execute a command on the local machine. Used
 * to restart applications.
 */
void KSMServer::startApplication( const QStringList& cmd, const QString& clientMachine,
    const QString& userId)
{
    QStringList command = cmd;
    if ( command.isEmpty() )
        return;
    if ( !userId.isEmpty()) {
        struct passwd* pw = getpwuid( getuid());
        if( pw != nullptr && userId != QString::fromLocal8Bit( pw->pw_name )) {
            command.prepend( QStringLiteral("--") );
            command.prepend( userId );
            command.prepend( QStringLiteral("-u") );
            command.prepend( QStandardPaths::findExecutable(QStringLiteral("kdesu")));
        }
    }
    if ( !clientMachine.isEmpty() && clientMachine != QLatin1String("localhost") ) {
        command.prepend( clientMachine );
        command.prepend( xonCommand ); // "xon" by default
    }

    const QString app = command.takeFirst();
    const QStringList argList = command;
    auto *job = new KIO::CommandLauncherJob(app, argList);
    job->start();
}

/*! Utility function to execute a command on the local machine. Used
 * to discard session data
 */
void KSMServer::executeCommand( const QStringList& command )
{
    if ( command.isEmpty() )
        return;

    KProcess::execute( command );
}

IceAuthDataEntry *authDataEntries = nullptr;

static QTemporaryFile *remTempFile = nullptr;

static IceListenObj *listenObjs = nullptr;
int numTransports = 0;
static bool only_local = 0;

static Bool HostBasedAuthProc ( char* /*hostname*/)
{
    if (only_local)
        return true;
    else
        return false;
}


Status KSMRegisterClientProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    char *              previousId
)
{
    KSMClient* client = (KSMClient*) managerData;
    client->registerClient( previousId );
    return 1;
}

void KSMInteractRequestProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    int                 dialogType
)
{
    the_server->interactRequest( (KSMClient*) managerData, dialogType );
}

void KSMInteractDoneProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    Bool                        cancelShutdown
)
{
    the_server->interactDone( (KSMClient*) managerData, cancelShutdown );
}

void KSMSaveYourselfRequestProc (
    SmsConn             smsConn ,
    SmPointer           /* managerData */,
    int                 saveType,
    Bool                shutdown,
    int                 interactStyle,
    Bool                fast,
    Bool                global
)
{
    if ( shutdown ) {
        the_server->shutdown( fast ?
                              KWorkSpace::ShutdownConfirmNo :
                              KWorkSpace::ShutdownConfirmDefault,
                              KWorkSpace::ShutdownTypeDefault,
                              KWorkSpace::ShutdownModeDefault );
    } else if ( !global ) {
        SmsSaveYourself( smsConn, saveType, false, interactStyle, fast );
        SmsSaveComplete( smsConn );
    }
    // else checkpoint only, ksmserver does not yet support this
    // mode. Will come for KDE 3.1
}

void KSMSaveYourselfPhase2RequestProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData
)
{
    the_server->phase2Request( (KSMClient*) managerData );
}

void KSMSaveYourselfDoneProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    Bool                success
)
{
    the_server->saveYourselfDone( (KSMClient*) managerData, success );
}

void KSMCloseConnectionProc (
    SmsConn             smsConn,
    SmPointer           managerData,
    int                 count,
    char **             reasonMsgs
)
{
    the_server->deleteClient( ( KSMClient* ) managerData );
    if ( count )
        SmFreeReasons( count, reasonMsgs );
    IceConn iceConn = SmsGetIceConnection( smsConn );
    SmsCleanUp( smsConn );
    IceSetShutdownNegotiation (iceConn, False);
    IceCloseConnection( iceConn );
}

void KSMSetPropertiesProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    int                 numProps,
    SmProp **           props
)
{
    KSMClient* client = ( KSMClient* ) managerData;
    for ( int i = 0; i < numProps; i++ ) {
        SmProp *p = client->property( props[i]->name );
        if ( p ) {
            client->properties.removeAll( p );
            SmFreeProperty( p );
        }
        client->properties.append( props[i] );
    }

    if ( numProps )
        free( props );

}

void KSMDeletePropertiesProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    int                 numProps,
    char **             propNames
)
{
    KSMClient* client = ( KSMClient* ) managerData;
    for ( int i = 0; i < numProps; i++ ) {
        SmProp *p = client->property( propNames[i] );
        if ( p ) {
            client->properties.removeAll( p );
            SmFreeProperty( p );
        }
    }
}

void KSMGetPropertiesProc (
    SmsConn             smsConn,
    SmPointer           managerData
)
{
    KSMClient* client = ( KSMClient* ) managerData;
    SmProp** props = new SmProp*[client->properties.count()];
    int i = 0;
    foreach( SmProp *prop, client->properties )
        props[i++] = prop;

    SmsReturnProperties( smsConn, i, props );
    delete [] props;
}


class KSMListener : public QSocketNotifier
{
public:
    KSMListener( IceListenObj obj )
        : QSocketNotifier( IceGetListenConnectionNumber( obj ),
                           QSocketNotifier::Read )
{
    listenObj = obj;
}

    IceListenObj listenObj;
};

class KSMConnection : public QSocketNotifier
{
 public:
  KSMConnection( IceConn conn )
    : QSocketNotifier( IceConnectionNumber( conn ),
                       QSocketNotifier::Read )
    {
        iceConn = conn;
    }

    IceConn iceConn;
};


/* for printing hex digits */
static void fprintfhex (FILE *fp, unsigned int len, char *cp)
{
    static const char hexchars[] = "0123456789abcdef";

    for (; len > 0; len--, cp++) {
        unsigned char s = *cp;
        putc(hexchars[s >> 4], fp);
        putc(hexchars[s & 0x0f], fp);
    }
}

/*
 * We use temporary files which contain commands to add/remove entries from
 * the .ICEauthority file.
 */
static void write_iceauth (FILE *addfp, FILE *removefp, IceAuthDataEntry *entry)
{
    fprintf (addfp,
             "add %s \"\" %s %s ",
             entry->protocol_name,
             entry->network_id,
             entry->auth_name);
    fprintfhex (addfp, entry->auth_data_length, entry->auth_data);
    fprintf (addfp, "\n");

    fprintf (removefp,
             "remove protoname=%s protodata=\"\" netid=%s authname=%s\n",
             entry->protocol_name,
             entry->network_id,
             entry->auth_name);
}


#define MAGIC_COOKIE_LEN 16

Status SetAuthentication_local (int count, IceListenObj *listenObjs)
{
    int i;
    for (i = 0; i < count; i ++) {
        char *prot = IceGetListenConnectionString(listenObjs[i]);
        if (!prot) continue;
        char *host = strchr(prot, '/');
        char *sock = nullptr;
        if (host) {
            *host=0;
            host++;
            sock = strchr(host, ':');
            if (sock) {
                *sock = 0;
                sock++;
            }
        }
        qCDebug(KSMSERVER) << "KSMServer: SetAProc_loc: conn " << (unsigned)i << ", prot=" << prot << ", file=" << sock;
        if (sock && !strcmp(prot, "local")) {
            chmod(sock, 0700);
        }
        IceSetHostBasedAuthProc (listenObjs[i], HostBasedAuthProc);
        free(prot);
    }
    return 1;
}

Status SetAuthentication (int count, IceListenObj *listenObjs,
                          IceAuthDataEntry **authDataEntries)
{
    QTemporaryFile addTempFile;
    remTempFile = new QTemporaryFile;

    if (!addTempFile.open() || !remTempFile->open())
        return 0;

    if ((*authDataEntries = (IceAuthDataEntry *) malloc (
                         count * 2 * sizeof (IceAuthDataEntry))) == nullptr)
        return 0;

    FILE *addAuthFile = fopen(QFile::encodeName(addTempFile.fileName()).constData(), "r+");
    FILE *remAuthFile = fopen(QFile::encodeName(remTempFile->fileName()).constData(), "r+");

    for (int i = 0; i < numTransports * 2; i += 2) {
        (*authDataEntries)[i].network_id =
            IceGetListenConnectionString (listenObjs[i/2]);
        (*authDataEntries)[i].protocol_name = (char *) "ICE";
        (*authDataEntries)[i].auth_name = (char *) "MIT-MAGIC-COOKIE-1";

        (*authDataEntries)[i].auth_data =
            IceGenerateMagicCookie (MAGIC_COOKIE_LEN);
        (*authDataEntries)[i].auth_data_length = MAGIC_COOKIE_LEN;

        (*authDataEntries)[i+1].network_id =
            IceGetListenConnectionString (listenObjs[i/2]);
        (*authDataEntries)[i+1].protocol_name = (char *) "XSMP";
        (*authDataEntries)[i+1].auth_name = (char *) "MIT-MAGIC-COOKIE-1";

        (*authDataEntries)[i+1].auth_data =
            IceGenerateMagicCookie (MAGIC_COOKIE_LEN);
        (*authDataEntries)[i+1].auth_data_length = MAGIC_COOKIE_LEN;

        write_iceauth (addAuthFile, remAuthFile, &(*authDataEntries)[i]);
        write_iceauth (addAuthFile, remAuthFile, &(*authDataEntries)[i+1]);

        IceSetPaAuthData (2, &(*authDataEntries)[i]);

        IceSetHostBasedAuthProc (listenObjs[i/2], HostBasedAuthProc);
    }
    fclose(addAuthFile);
    fclose(remAuthFile);

    QString iceAuth = QStandardPaths::findExecutable(QStringLiteral("iceauth"));
    if (iceAuth.isEmpty())
    {
        qCWarning(KSMSERVER, "KSMServer: could not find iceauth");
        return 0;
    }

    KProcess p;
    p << iceAuth << QStringLiteral("source") << addTempFile.fileName();
    p.execute();

    return (1);
}

/*
 * Free up authentication data.
 */
void FreeAuthenticationData(int count, IceAuthDataEntry *authDataEntries)
{
    /* Each transport has entries for ICE and XSMP */
    if (only_local)
        return;

    for (int i = 0; i < count * 2; i++) {
        free (authDataEntries[i].network_id);
        free (authDataEntries[i].auth_data);
    }

    free (authDataEntries);

    QString iceAuth = QStandardPaths::findExecutable(QStringLiteral("iceauth"));
    if (iceAuth.isEmpty())
    {
        qCWarning(KSMSERVER, "KSMServer: could not find iceauth");
        return;
    }

    if (remTempFile)
    {
        KProcess p;
        p << iceAuth << QStringLiteral("source") << remTempFile->fileName();
        p.execute();
    }

    delete remTempFile;
    remTempFile = nullptr;
}

static int Xio_ErrorHandler( Display * )
{
    qCWarning(KSMSERVER, "ksmserver: Fatal IO error: client killed");

    // Don't do anything that might require the X connection
    if (the_server)
    {
       KSMServer *server = the_server;
       the_server = nullptr;
       server->cleanUp();
       // Don't delete server!!
    }

    exit(0); // Don't report error, it's not our fault.
    return 0; // Bogus return value, notreached
}

void KSMServer::setupXIOErrorHandler()
{
    XSetIOErrorHandler(Xio_ErrorHandler);
}

static int wake_up_socket = -1;
static void sighandler(int sig)
{
    if (sig == SIGHUP) {
        signal(SIGHUP, sighandler);
        return;
    }

    char ch = 0;
    (void)::write(wake_up_socket, &ch, 1);
}


void KSMWatchProc ( IceConn iceConn, IcePointer client_data, Bool opening, IcePointer* watch_data)
{
    KSMServer* ds = ( KSMServer*) client_data;

    if (opening) {
        *watch_data = (IcePointer) ds->watchConnection( iceConn );
    }
    else  {
        ds->removeConnection( (KSMConnection*) *watch_data );
    }
}

static Status KSMNewClientProc ( SmsConn conn, SmPointer manager_data,
                                 unsigned long* mask_ret, SmsCallbacks* cb, char** failure_reason_ret)
{
    *failure_reason_ret = nullptr;

    void* client =  ((KSMServer*) manager_data )->newClient( conn );

    cb->register_client.callback = KSMRegisterClientProc;
    cb->register_client.manager_data = client;
    cb->interact_request.callback = KSMInteractRequestProc;
    cb->interact_request.manager_data = client;
    cb->interact_done.callback = KSMInteractDoneProc;
    cb->interact_done.manager_data = client;
    cb->save_yourself_request.callback = KSMSaveYourselfRequestProc;
    cb->save_yourself_request.manager_data = client;
    cb->save_yourself_phase2_request.callback = KSMSaveYourselfPhase2RequestProc;
    cb->save_yourself_phase2_request.manager_data = client;
    cb->save_yourself_done.callback = KSMSaveYourselfDoneProc;
    cb->save_yourself_done.manager_data = client;
    cb->close_connection.callback = KSMCloseConnectionProc;
    cb->close_connection.manager_data = client;
    cb->set_properties.callback = KSMSetPropertiesProc;
    cb->set_properties.manager_data = client;
    cb->delete_properties.callback = KSMDeletePropertiesProc;
    cb->delete_properties.manager_data = client;
    cb->get_properties.callback = KSMGetPropertiesProc;
    cb->get_properties.manager_data = client;

    *mask_ret = SmsRegisterClientProcMask |
                SmsInteractRequestProcMask |
                SmsInteractDoneProcMask |
                SmsSaveYourselfRequestProcMask |
                SmsSaveYourselfP2RequestProcMask |
                SmsSaveYourselfDoneProcMask |
                SmsCloseConnectionProcMask |
                SmsSetPropertiesProcMask |
                SmsDeletePropertiesProcMask |
                SmsGetPropertiesProcMask;
    return 1;
}


#ifdef HAVE__ICETRANSNOLISTEN
extern "C" int _IceTransNoListen(const char * protocol);
#endif

KSMServer::KSMServer(InitFlags flags)
  : sessionGroup( QStringLiteral( "" ) )
  , m_kwinInterface(new OrgKdeKWinSessionInterface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Session"), QDBusConnection::sessionBus(), this))
  , sockets{ -1, -1 }
{
    if (!flags.testFlag(InitFlag::NoLockScreen)) {
        ScreenLocker::KSldApp::self()->initialize();
        if (flags.testFlag(InitFlag::ImmediateLockScreen)) {
            ScreenLocker::KSldApp::self()->lock(ScreenLocker::EstablishLock::Immediate);
        }
    }

    if(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) != 0)
        qFatal("Could not create socket pair, error %d (%s)", errno, strerror(errno));
    wake_up_socket = sockets[0];
    QSocketNotifier* n = new QSocketNotifier(sockets[1], QSocketNotifier::Read, this);
    qApp->connect(n, &QSocketNotifier::activated, &QApplication::quit);

    new KSMServerInterfaceAdaptor( this );
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/KSMServer"), this);
    the_server = this;
    clean = false;

    state = Idle;
    saveSession = false;
    KConfigGroup config(KSharedConfig::openConfig(), "General");
    clientInteracting = nullptr;
    xonCommand = config.readEntry( "xonCommand", "xon" );

    only_local = flags.testFlag(InitFlag::OnlyLocal);
#ifdef HAVE__ICETRANSNOLISTEN
    if (only_local)
        _IceTransNoListen("tcp");
#else
    only_local = false;
#endif

    char        errormsg[256];
    if (!SmsInitialize ( (char*) KSMVendorString, (char*) KSMReleaseString,
                         KSMNewClientProc,
                         (SmPointer) this,
                         HostBasedAuthProc, 256, errormsg ) ) {

        qCWarning(KSMSERVER, "KSMServer: could not register XSM protocol");
    }

    if (!IceListenForConnections (&numTransports, &listenObjs,
                                  256, errormsg))
    {
        qCWarning(KSMSERVER, "KSMServer: Error listening for connections: %s", errormsg);
        qCWarning(KSMSERVER, "KSMServer: Aborting.");
        exit(1);
    }

    {
        // publish available transports.
        QByteArray fName = QFile::encodeName(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
                                             + QDir::separator()
                                             + QStringLiteral("KSMserver"));
        qCDebug(KSMSERVER) << fName;
        QString display = QString::fromLocal8Bit(::getenv("DISPLAY"));
        // strip the screen number from the display
        display.remove(QRegularExpression(QStringLiteral("\\.[0-9]+$")));
        int i;
        while( (i = display.indexOf(QLatin1Char(':'))) >= 0)
           display[i] = '_';
        while( (i = display.indexOf(QLatin1Char('/'))) >= 0)
           display[i] = '_';

        fName += '_'+display.toLocal8Bit();
        FILE *f;
        f = ::fopen(fName.data(), "w+");
        if (!f)
        {
            qCWarning(KSMSERVER, "KSMServer: cannot open %s: %s", fName.data(), strerror(errno));
            qCWarning(KSMSERVER, "KSMServer: Aborting.");
            exit(1);
        }
        char* session_manager = IceComposeNetworkIdList(numTransports, listenObjs);
        fprintf(f, "%s\n%i\n", session_manager, getpid());
        fclose(f);
        setenv( "SESSION_MANAGER", session_manager, true  );

        auto updateEnvJob = new UpdateLaunchEnvJob(QStringLiteral("SESSION_MANAGER"), QString::fromLatin1(session_manager));
        updateEnvJob->start();

        free(session_manager);
    }

    if (only_local) {
        if (!SetAuthentication_local(numTransports, listenObjs))
            qFatal("KSMSERVER: authentication setup failed.");
    } else {
        if (!SetAuthentication(numTransports, listenObjs, &authDataEntries))
            qFatal("KSMSERVER: authentication setup failed.");
    }

    IceAddConnectionWatch (KSMWatchProc, (IcePointer) this);

    KSMListener* con;
    for ( int i = 0; i < numTransports; i++) {
        fcntl( IceGetListenConnectionNumber( listenObjs[i] ), F_SETFD, FD_CLOEXEC );
        con = new KSMListener( listenObjs[i] );
        listener.append( con );
        connect(con, &KSMListener::activated, this, &KSMServer::newConnection);
    }

    signal(SIGHUP, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGPIPE, SIG_IGN);

    connect(&protectionTimer, &QTimer::timeout, this, &KSMServer::protectionTimeout);
    connect(&restoreTimer, &QTimer::timeout, this, &KSMServer::tryRestoreNext);
    connect(this, &KSMServer::sessionRestored, this, [this]() {
        auto reply = m_restoreSessionCall.createReply();
        QDBusConnection::sessionBus().send(reply);
        m_restoreSessionCall = QDBusMessage();
    });
    connect(qApp, &QApplication::aboutToQuit, this, &KSMServer::cleanUp);

    setupXIOErrorHandler();
}

KSMServer::~KSMServer()
{
    qDeleteAll( listener );
    the_server = nullptr;
    cleanUp();
}

void KSMServer::cleanUp()
{
    if (clean) return;
    clean = true;
    IceFreeListenObjs (numTransports, listenObjs);

    wake_up_socket = -1;
    ::close(sockets[1]);
    ::close(sockets[0]);
    sockets[0] = -1;
    sockets[1] = -1;

    QByteArray fName = QFile::encodeName(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + QLatin1Char('/') + QStringLiteral("KSMserver"));
    QString display  = QString::fromLocal8Bit(::getenv("DISPLAY"));
    // strip the screen number from the display
    display.remove(QRegularExpression(QStringLiteral("\\.[0-9]+$")));
    int i;
    while( (i = display.indexOf(QLatin1Char(':'))) >= 0)
         display[i] = '_';
    while( (i = display.indexOf(QLatin1Char('/'))) >= 0)
         display[i] = '_';

    fName += '_'+display.toLocal8Bit();
    ::unlink(fName.data());

    FreeAuthenticationData(numTransports, authDataEntries);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
}



void* KSMServer::watchConnection( IceConn iceConn )
{
    KSMConnection* conn = new KSMConnection( iceConn );
    connect(conn, &KSMConnection::activated, this, &KSMServer::processData);
    return (void*) conn;
}

void KSMServer::removeConnection( KSMConnection* conn )
{
    delete conn;
}


/*!
  Called from our IceIoErrorHandler
 */
void KSMServer::ioError( IceConn /*iceConn*/  )
{
}

void KSMServer::processData( int /*socket*/ )
{
    IceConn iceConn = ((KSMConnection*)sender())->iceConn;
    IceProcessMessagesStatus status = IceProcessMessages( iceConn, nullptr, nullptr );
    if ( status == IceProcessMessagesIOError ) {
        IceSetShutdownNegotiation( iceConn, False );
        QList<KSMClient*>::iterator it = clients.begin();
        QList<KSMClient*>::iterator const itEnd = clients.end();
        while ( ( it != itEnd ) && *it && ( SmsGetIceConnection( ( *it )->connection() ) != iceConn ) )
            ++it;
        if ( ( it != itEnd ) && *it ) {
            SmsConn smsConn = (*it)->connection();
            deleteClient( *it );
            SmsCleanUp( smsConn );
        }
        (void) IceCloseConnection( iceConn );
    }
}

KSMClient* KSMServer::newClient( SmsConn conn )
{
    KSMClient* client = new KSMClient( conn );
    clients.append( client );
    return client;
}

void KSMServer::deleteClient( KSMClient* client )
{
    if ( !clients.contains( client ) ) // paranoia
        return;
    clients.removeAll( client );
    clientsToKill.removeAll( client );
    clientsToSave.removeAll( client );
    if ( client == clientInteracting ) {
        clientInteracting = nullptr;
        handlePendingInteractions();
    }
    delete client;
    if ( state == Shutdown || state == Checkpoint || state == ClosingSubSession )
        completeShutdownOrCheckpoint();
    if ( state == Killing )
        completeKilling();
    else if ( state == KillingSubSession )
        completeKillingSubSession();
}

void KSMServer::newConnection( int /*socket*/ )
{
    IceAcceptStatus status;
    IceConn iceConn = IceAcceptConnection( ((KSMListener*)sender())->listenObj, &status);
    if( iceConn == nullptr )
        return;
    IceSetShutdownNegotiation( iceConn, False );
    IceConnectStatus cstatus;
    while ((cstatus = IceConnectionStatus (iceConn))==IceConnectPending) {
        (void) IceProcessMessages( iceConn, nullptr, nullptr );
    }

    if (cstatus != IceConnectAccepted) {
        if (cstatus == IceConnectIOError)
            qCDebug(KSMSERVER) << "IO error opening ICE Connection!";
        else
            qCDebug(KSMSERVER) << "ICE Connection rejected!";
        (void )IceCloseConnection (iceConn);
        return;
    }

    // don't leak the fd
    fcntl( IceConnectionNumber(iceConn), F_SETFD, FD_CLOEXEC );
}

QString KSMServer::currentSession()
{
    if ( sessionGroup.startsWith( QLatin1String( "Session: " ) ) )
        return sessionGroup.mid( 9 );
    return QStringLiteral( "" ); // empty, not null, since used for KConfig::setGroup // TODO does this comment make any sense?
}

void KSMServer::discardSession()
{
    KConfigGroup config(KSharedConfig::openConfig(), sessionGroup );
    int count =  config.readEntry( "count", 0 );
	foreach ( KSMClient *c, clients ) {
        QStringList discardCommand = c->discardCommand();
        if ( discardCommand.isEmpty())
            continue;
        // check that non of the old clients used the exactly same
        // discardCommand before we execute it. This used to be the
        // case up to KDE and Qt < 3.1
        int i = 1;
        while ( i <= count &&
                config.readPathEntry( QStringLiteral("discardCommand") + QString::number(i), QStringList() ) != discardCommand )
            i++;
        if ( i <= count )
            executeCommand( discardCommand );
    }
}

void KSMServer::storeSession()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration(); // config may have changed in the KControl module
    KConfigGroup generalGroup(config, "General");
    excludeApps = generalGroup.readEntry( "excludeApps" ).toLower()
                  .split( QRegularExpression( QStringLiteral("[,:]") ), QString::SkipEmptyParts );
    KConfigGroup configSessionGroup(config, sessionGroup);
    int count =  configSessionGroup.readEntry( "count", 0 );
    for ( int i = 1; i <= count; i++ ) {
        QStringList discardCommand = configSessionGroup.readPathEntry( QLatin1String("discardCommand") + QString::number(i), QStringList() );
        if ( discardCommand.isEmpty())
            continue;
        // check that non of the new clients uses the exactly same
        // discardCommand before we execute it. This used to be the
        // case up to KDE and Qt < 3.1
        QList<KSMClient*>::iterator it = clients.begin();
        QList<KSMClient*>::iterator const itEnd = clients.end();
        while ( ( it != itEnd ) && *it && (discardCommand != ( *it )->discardCommand() ) )
            ++it;
        if ( ( it != itEnd ) && *it )
            continue;
        executeCommand( discardCommand );
    }
    config->deleteGroup( sessionGroup ); //### does not work with global config object...
	KConfigGroup cg( config, sessionGroup);
    count =  0;

    // Tell kwin to save its state
    auto reply = m_kwinInterface->finishSaveSession(currentSession());
    reply.waitForFinished(); // boo!

    foreach ( KSMClient *c, clients ) {
        int restartHint = c->restartStyleHint();
        if (restartHint == SmRestartNever)
           continue;
        QString program = c->program();
        QStringList restartCommand = c->restartCommand();
        if (program.isEmpty() && restartCommand.isEmpty())
           continue;
        if (state == ClosingSubSession && ! clientsToSave.contains(c))
            continue;

        // 'program' might be (mostly) fullpath, or (sometimes) just the name.
        // 'name' is just the name.
        QFileInfo info(program);
        const QString& name = info.fileName();

        if ( excludeApps.contains(program.toLower()) ||
             excludeApps.contains(name.toLower()) ) {
            continue;
        }

        count++;
        QString n = QString::number(count);
        cg.writeEntry( QStringLiteral("program")+n, program );
        cg.writeEntry( QStringLiteral("clientId")+n, c->clientId() );
        cg.writeEntry( QStringLiteral("restartCommand")+n, restartCommand );
        cg.writePathEntry( QStringLiteral("discardCommand")+n, c->discardCommand() );
        cg.writeEntry( QStringLiteral("restartStyleHint")+n, restartHint );
        cg.writeEntry( QStringLiteral("userId")+n, c->userId() );
    }
    cg.writeEntry( "count", count );

    KConfigGroup cg2( config, "General");

    storeLegacySession(config.data());
    config->sync();
}

QStringList KSMServer::sessionList()
{
    QStringList sessions( QStringLiteral( "default" ) );
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    const QStringList groups = config->groupList();
    for ( QStringList::ConstIterator it = groups.constBegin(); it != groups.constEnd(); ++it )
        if ( (*it).startsWith( QLatin1String( "Session: " ) ) )
            sessions << (*it).mid( 9 );
    return sessions;
}

bool KSMServer::defaultSession() const
{
    return sessionGroup.isEmpty();
}

void KSMServer::setupShortcuts()
{
    if (KAuthorized::authorize( QStringLiteral( "logout" ))) {
        KActionCollection* actionCollection = new KActionCollection(this);
        actionCollection->setComponentDisplayName(i18n("Session Management"));
        QAction* a;
        a = actionCollection->addAction(QStringLiteral("Log Out"));
        a->setText(i18n("Log Out"));
        KGlobalAccel::self()->setGlobalShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::Key_Delete);
        connect(a, &QAction::triggered, this, &KSMServer::defaultLogout);

        a = actionCollection->addAction(QStringLiteral("Log Out Without Confirmation"));
        a->setText(i18n("Log Out Without Confirmation"));
        KGlobalAccel::self()->setGlobalShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::SHIFT+Qt::Key_Delete);
        connect(a, &QAction::triggered, this, &KSMServer::logoutWithoutConfirmation);

        a = actionCollection->addAction(QStringLiteral("Halt Without Confirmation"));
        a->setText(i18n("Halt Without Confirmation"));
        KGlobalAccel::self()->setGlobalShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::SHIFT+Qt::Key_PageDown);
        connect(a, &QAction::triggered, this, &KSMServer::haltWithoutConfirmation);

        a = actionCollection->addAction(QStringLiteral("Reboot Without Confirmation"));
        a->setText(i18n("Reboot Without Confirmation"));
        KGlobalAccel::self()->setGlobalShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::SHIFT+Qt::Key_PageUp);
        connect(a, &QAction::triggered, this, &KSMServer::rebootWithoutConfirmation);
    }
}

/*!  Restores the previous session.
 */
void KSMServer::restoreSession( const QString &sessionName )
{
    if( state != Idle )
        return;
#ifdef KSMSERVER_STARTUP_DEBUG1
    t.start();
#endif
    state = RestoringWMSession;

    qCDebug(KSMSERVER) << "KSMServer::restoreSession " << sessionName;
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    sessionGroup = QLatin1String("Session: ") + sessionName;
    KConfigGroup configSessionGroup( config, sessionGroup);

    int count =  configSessionGroup.readEntry( "count", 0 );
    appsToStart = count;

    auto reply = m_kwinInterface->loadSession(sessionName);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (state == RestoringWMSession) {
            state = Idle;
        }
    });
}

/*!
  Starts the default session.
 */
void KSMServer::startDefaultSession()
{
    if( state != Idle )
        return;
    state = RestoringWMSession;
#ifdef KSMSERVER_STARTUP_DEBUG1
    t.start();
#endif
    sessionGroup = QString();
}

void KSMServer::restoreSession()
{
    Q_ASSERT(calledFromDBus());
    if (defaultSession()) {
        state = KSMServer::Idle;
        return;
   }

   setDelayedReply(true);
   m_restoreSessionCall = message();

   restoreLegacySession(KSharedConfig::openConfig().data());
   lastAppStarted = 0;
   lastIdStarted.clear();
   state = KSMServer::Restoring;
   tryRestoreNext();
}

void KSMServer::restoreSubSession( const QString& name )
{
    sessionGroup = QStringLiteral( "SubSession: " ) + name;

    KConfigGroup configSessionGroup( KSharedConfig::openConfig(), sessionGroup);
    int count =  configSessionGroup.readEntry( "count", 0 );
    appsToStart = count;
    lastAppStarted = 0;
    lastIdStarted.clear();

    state = RestoringSubSession;
    tryRestoreNext();
}

void KSMServer::clientRegistered( const char* previousId )
{
    if ( previousId && lastIdStarted == QString::fromLocal8Bit( previousId ) )
        tryRestoreNext();
}

void KSMServer::tryRestoreNext()
{
    if( state != Restoring && state != RestoringSubSession )
        return;
    restoreTimer.stop();
    KConfigGroup config(KSharedConfig::openConfig(), sessionGroup );

    while ( lastAppStarted < appsToStart ) {
        lastAppStarted++;
        QString n = QString::number(lastAppStarted);
        QString clientId = config.readEntry( QLatin1String("clientId")+n, QString() );
        bool alreadyStarted = false;
        foreach ( KSMClient *c, clients ) {
            if ( QString::fromLocal8Bit( c->clientId() ) == clientId ) {
                alreadyStarted = true;
                break;
            }
        }
        if ( alreadyStarted )
            continue;

        QStringList restartCommand = config.readEntry( QLatin1String("restartCommand")+n, QStringList() );
        if ( restartCommand.isEmpty() ||
             (config.readEntry( QStringLiteral("restartStyleHint")+n, 0 ) == SmRestartNever)) {
            continue;
        }
        startApplication( restartCommand,
                          config.readEntry( QStringLiteral("clientMachine")+n, QString() ),
                          config.readEntry( QStringLiteral("userId")+n, QString() ));
        lastIdStarted = clientId;
        if ( !lastIdStarted.isEmpty() ) {
            restoreTimer.setSingleShot( true );
            restoreTimer.start( 2000 );
            return; // we get called again from the clientRegistered handler
        }
    }

    //all done
    appsToStart = 0;
    lastIdStarted.clear();

    if (state == Restoring) {
        emit sessionRestored();
    } else { //subsession
        emit subSessionOpened();
    }
    state = Idle;
}

void KSMServer::startupDone()
{
    state = Idle;
}


void KSMServer::defaultLogout()
{
    shutdown(KWorkSpace::ShutdownConfirmYes, KWorkSpace::ShutdownTypeDefault, KWorkSpace::ShutdownModeDefault);
}

void KSMServer::logoutWithoutConfirmation()
{
    shutdown(KWorkSpace::ShutdownConfirmNo, KWorkSpace::ShutdownTypeNone, KWorkSpace::ShutdownModeDefault);
}

void KSMServer::haltWithoutConfirmation()
{
    shutdown(KWorkSpace::ShutdownConfirmNo, KWorkSpace::ShutdownTypeHalt, KWorkSpace::ShutdownModeDefault);
}

void KSMServer::rebootWithoutConfirmation()
{
    shutdown(KWorkSpace::ShutdownConfirmNo, KWorkSpace::ShutdownTypeReboot, KWorkSpace::ShutdownModeDefault);
}

void KSMServer::openSwitchUserDialog()
{
    //this method exists only for compatibility. Users should ideally call this directly
    OrgKdeScreensaverInterface iface(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus());
    iface.SwitchUser();
}
