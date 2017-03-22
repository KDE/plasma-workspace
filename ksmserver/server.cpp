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
#include <QRegExp>
#include <QtDBus/QtDBus>
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

#include <KScreenLocker/KsldApp>

#include <kdisplaymanager.h>
#include <QX11Info>
#include <krandom.h>
#include <klauncher_interface.h>
#include <qstandardpaths.h>

#include "switchuserdialog.h"

KSMServer* the_server = 0;

KSMServer* KSMServer::self()
{
    return the_server;
}

/*! Utility function to execute a command on the local machine. Used
 * to restart applications.
 */
KProcess* KSMServer::startApplication( const QStringList& cmd, const QString& clientMachine,
    const QString& userId, bool wm )
{
    QStringList command = cmd;
    if ( command.isEmpty() )
        return NULL;
    if ( !userId.isEmpty()) {
        struct passwd* pw = getpwuid( getuid());
        if( pw != NULL && userId != QString::fromLocal8Bit( pw->pw_name )) {
            command.prepend( QStringLiteral("--") );
            command.prepend( userId );
            command.prepend( QStringLiteral("-u") );
            command.prepend( QStandardPaths::findExecutable(QStringLiteral("kdesu")));
        }
    }
    if ( !clientMachine.isEmpty() && clientMachine != QStringLiteral("localhost") ) {
        command.prepend( clientMachine );
        command.prepend( xonCommand ); // "xon" by default
    }

// TODO this function actually should not use KProcess at all and use klauncher (kdeinit) instead.
// Klauncher should also have support for tracking whether the launched process is still alive
// or not, so this should be redone. For now, use KProcess for wm's, as they need to be tracked,
// klauncher for the rest where ksmserver doesn't care.
    if( wm ) {
        KProcess* process = new KProcess( this );
        *process << command;
        // make it auto-delete
        connect(process, static_cast<void (KProcess::*)(QProcess::ProcessError)>(&KProcess::error), process, &KProcess::deleteLater);
        connect(process, static_cast<void (KProcess::*)(int, QProcess::ExitStatus)>(&KProcess::finished), process, &KProcess::deleteLater);
        process->start();
        return process;
    } else {
        int n = command.count();
        org::kde::KLauncher klauncher(QStringLiteral("org.kde.klauncher5"),
                                      QStringLiteral("/KLauncher"), QDBusConnection::sessionBus());
        QString app = command[0];
        QStringList argList;
        for ( int i=1; i < n; i++)
           argList.append( command[i]);
        klauncher.exec_blind(app, argList );
        return NULL;
    }
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

IceAuthDataEntry *authDataEntries = 0;

static QTemporaryFile *remTempFile = 0;

static IceListenObj *listenObjs = 0;
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
        if ( !qstrcmp( props[i]->name, SmProgram ) )
            the_server->clientSetProgram( client );
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
        char *sock = 0;
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
                         count * 2 * sizeof (IceAuthDataEntry))) == NULL)
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
        qWarning("KSMServer: could not find iceauth");
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
        qWarning("KSMServer: could not find iceauth");
        return;
    }

    if (remTempFile)
    {
        KProcess p;
        p << iceAuth << QStringLiteral("source") << remTempFile->fileName();
        p.execute();
    }

    delete remTempFile;
    remTempFile = 0;
}

static int Xio_ErrorHandler( Display * )
{
    qWarning("ksmserver: Fatal IO error: client killed");

    // Don't do anything that might require the X connection
    if (the_server)
    {
       KSMServer *server = the_server;
       the_server = 0;
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

static void sighandler(int sig)
{
    if (sig == SIGHUP) {
        signal(SIGHUP, sighandler);
        return;
    }

    if (the_server)
    {
       KSMServer *server = the_server;
       the_server = 0;
       server->cleanUp();
       delete server;
    }

    if (qApp)
        qApp->quit();
    //::exit(0);
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
    *failure_reason_ret = 0;

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

KSMServer::KSMServer( const QString& windowManager, InitFlags flags )
  : wmProcess( NULL )
  , sessionGroup( QStringLiteral( "" ) )
  , logoutEffectWidget( NULL )
{
    if (!flags.testFlag(InitFlag::NoLockScreen)) {
        ScreenLocker::KSldApp::self()->initialize();
        if (flags.testFlag(InitFlag::ImmediateLockScreen)) {
            ScreenLocker::KSldApp::self()->lock(ScreenLocker::EstablishLock::Immediate);
        }
    }

    new KSMServerInterfaceAdaptor( this );
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/KSMServer"), this);
    kcminitSignals = NULL;
    the_server = this;
    clean = false;

    shutdownType = KWorkSpace::ShutdownTypeNone;

    state = Idle;
    dialogActive = false;
    saveSession = false;
    wmPhase1WaitingCount = 0;
    KConfigGroup config(KSharedConfig::openConfig(), "General");
    clientInteracting = 0;
    xonCommand = config.readEntry( "xonCommand", "xon" );

    selectWm( windowManager );

    connect(&startupSuspendTimeoutTimer, &QTimer::timeout, this, &KSMServer::startupSuspendTimeout);
    connect(&pendingShutdown, &QTimer::timeout, this, &KSMServer::pendingShutdownTimeout);

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

        qWarning("KSMServer: could not register XSM protocol");
    }

    if (!IceListenForConnections (&numTransports, &listenObjs,
                                  256, errormsg))
    {
        qWarning("KSMServer: Error listening for connections: %s", errormsg);
        qWarning("KSMServer: Aborting.");
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
        display.replace(QRegExp(QStringLiteral("\\.[0-9]+$")), QStringLiteral(""));
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
            qWarning("KSMServer: cannot open %s: %s", fName.data(), strerror(errno));
            qWarning("KSMServer: Aborting.");
            exit(1);
        }
        char* session_manager = IceComposeNetworkIdList(numTransports, listenObjs);
        fprintf(f, "%s\n%i\n", session_manager, getpid());
        fclose(f);
        setenv( "SESSION_MANAGER", session_manager, true  );

        // Pass env. var to kdeinit.
        org::kde::KLauncher klauncher( QStringLiteral( "org.kde.klauncher5" ), QStringLiteral( "/KLauncher" ), QDBusConnection::sessionBus());
        klauncher.setLaunchEnv( QStringLiteral( "SESSION_MANAGER" ), QString::fromLocal8Bit( (const char*) session_manager ) );

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
    connect(qApp, &QApplication::aboutToQuit, this, &KSMServer::cleanUp);
}

KSMServer::~KSMServer()
{
    qDeleteAll( listener );
    the_server = 0;
    cleanUp();
}

void KSMServer::cleanUp()
{
    if (clean) return;
    clean = true;
    IceFreeListenObjs (numTransports, listenObjs);

    QByteArray fName = QFile::encodeName(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + QLatin1Char('/') + QStringLiteral("KSMserver"));
    QString display  = QString::fromLocal8Bit(::getenv("DISPLAY"));
    // strip the screen number from the display
    display.replace(QRegExp(QStringLiteral("\\.[0-9]+$")), QStringLiteral(""));
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

    runShutdownScripts();

    KDisplayManager().shutdown( shutdownType, shutdownMode, bootOption );
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
    IceProcessMessagesStatus status = IceProcessMessages( iceConn, 0, 0 );
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
        clientInteracting = 0;
        handlePendingInteractions();
    }
    delete client;
    if ( state == Shutdown || state == Checkpoint || state == ClosingSubSession )
        completeShutdownOrCheckpoint();
    if ( state == Killing )
        completeKilling();
    else if ( state == KillingSubSession )
        completeKillingSubSession();
    if ( state == KillingWM )
        completeKillingWM();
}

void KSMServer::newConnection( int /*socket*/ )
{
    IceAcceptStatus status;
    IceConn iceConn = IceAcceptConnection( ((KSMListener*)sender())->listenObj, &status);
    if( iceConn == NULL )
        return;
    IceSetShutdownNegotiation( iceConn, False );
    IceConnectStatus cstatus;
    while ((cstatus = IceConnectionStatus (iceConn))==IceConnectPending) {
        (void) IceProcessMessages( iceConn, 0, 0 );
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
    return QStringLiteral( "" ); // empty, not null, since used for KConfig::setGroup
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
                  .split( QRegExp( QStringLiteral("[,:]") ), QString::SkipEmptyParts );
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

    if (state != ClosingSubSession) {
        // put the wm first
        foreach ( KSMClient *c, clients )
            if ( c->program() == wm ) {
                clients.removeAll( c );
                clients.prepend( c );
                break;
            }
    }

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
        cg.writeEntry( QStringLiteral("wasWm")+n, isWM( c ));
    }
    cg.writeEntry( "count", count );

    KConfigGroup cg2( config, "General");
    cg2.writeEntry( "screenCount", ScreenCount(QX11Info::display()));

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

bool KSMServer::isWM( const KSMClient* client ) const
{
    return isWM( client->program());
}

bool KSMServer::isWM( const QString& program ) const
{
    // Strip possible paths, so that even /usr/bin/kwin is recognized as kwin.
    QString wmName = wm.mid( wm.lastIndexOf( QDir::separator()) + 1 );
    QString programName = program.mid( program.lastIndexOf( QDir::separator()) + 1 );
    return programName == wmName;
}

bool KSMServer::defaultSession() const
{
    return sessionGroup.isEmpty();
}

// selection logic:
// - $KDEWM is set - use that
// - a wm is selected using the kcm - use that
// - if that fails, just use KWin
void KSMServer::selectWm( const QString& kdewm )
{
    wm = QStringLiteral( KWIN_BIN ); // defaults
    wmCommands = ( QStringList() << QStringLiteral( KWIN_BIN ) );
    if( !kdewm.isEmpty())
    {
        wmCommands = ( QStringList() << kdewm );
        wm = kdewm;
        return;
    }
    KConfigGroup config(KSharedConfig::openConfig(), "General");

    QString cfgwm = config.readEntry( "windowManager", "kwin" );
    KDesktopFile file( QStandardPaths::AppDataLocation,
                       QStringLiteral( "windowmanagers/" ) + cfgwm + QStringLiteral( ".desktop" ) );

    if( file.noDisplay())
        return;
    if( !file.tryExec())
        return;
    QString testexec = file.desktopGroup().readEntry( "X-KDE-WindowManagerTestExec" );
    if( !testexec.isEmpty())
    {
        KProcess proc;
        proc.setShellCommand( testexec );
        if( proc.execute() != 0 )
            return;
    }
    QStringList cfgWmCommands = KShell::splitArgs( file.desktopGroup().readEntry( "Exec" ));
    if( cfgWmCommands.isEmpty())
        return;
    QString smname = file.desktopGroup().readEntry( "X-KDE-WindowManagerId" );
    // ok
    wm = smname.isEmpty() ? cfgwm : smname;
    wmCommands = cfgWmCommands;
}

void KSMServer::wmChanged()
{
    KSharedConfig::openConfig()->reparseConfiguration();
    selectWm( QStringLiteral( "" ) );
}

void KSMServer::setupShortcuts()
{
    if (KAuthorized::authorize( QStringLiteral( "logout" ))) {
        KActionCollection* actionCollection = new KActionCollection(this);
        QAction* a;
        a = actionCollection->addAction(QStringLiteral("Log Out"));
        a->setText(i18n("Log Out"));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::Key_Delete);
        connect(a, &QAction::triggered, this, &KSMServer::defaultLogout);

        a = actionCollection->addAction(QStringLiteral("Log Out Without Confirmation"));
        a->setText(i18n("Log Out Without Confirmation"));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::SHIFT+Qt::Key_Delete);
        connect(a, &QAction::triggered, this, &KSMServer::logoutWithoutConfirmation);

        a = actionCollection->addAction(QStringLiteral("Halt Without Confirmation"));
        a->setText(i18n("Halt Without Confirmation"));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::SHIFT+Qt::Key_PageDown);
        connect(a, &QAction::triggered, this, &KSMServer::haltWithoutConfirmation);

        a = actionCollection->addAction(QStringLiteral("Reboot Without Confirmation"));
        a->setText(i18n("Reboot Without Confirmation"));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::SHIFT+Qt::Key_PageUp);
        connect(a, &QAction::triggered, this, &KSMServer::rebootWithoutConfirmation);
    }
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
    KDisplayManager dm;
    if (!dm.isSwitchable()) {
        return;
    }

    QScopedPointer<KSMSwitchUserDialog> dlg(new KSMSwitchUserDialog(&dm));
    dlg->exec();
}

void KSMServer::runShutdownScripts()
{
    const QStringList shutdownFolders = QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation, QStringLiteral("plasma-workspace/shutdown"), QStandardPaths::LocateDirectory);
    foreach (const QString &shutDownFolder, shutdownFolders) {
        QDir dir(shutDownFolder);
        if (!dir.exists()) {
            continue;
        }

        const QStringList entries = dir.entryList(QDir::Files);
        foreach (const QString &file, entries) {
            // Don't execute backup files
            if (!file.endsWith(QLatin1Char('~')) && !file.endsWith(QStringLiteral(".bak")) &&
                    (file[0] != QLatin1Char('%') || !file.endsWith(QLatin1Char('%'))) &&
                    (file[0] != QLatin1Char('#') || !file.endsWith(QLatin1Char('#'))))
            {
                const QString fullPath = dir.absolutePath() + QLatin1Char('/') + file;

                qCDebug(KSMSERVER) << "running shutdown script" << fullPath;
                QProcess::execute(fullPath);
            }
        }
    }
}
