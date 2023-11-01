/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 2005 Lubos Lunak <l.lunak@kde.org>
    SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

    SPDX-FileContributor: Oswald Buddenhagen <ob6@inf.tu-dresden.de>

    some code taken from the dcopserver (part of the KDE libraries), which is
    SPDX-FileCopyrightText: 1999 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 1999 Preston Brown <pbrown@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "server.h"
#include "client.h"
#include "global.h"
#include "klocalizedstring.h"
#include "ksmserver_debug.h"
#include "ksmserverinterfaceadaptor.h"

#include <config-ksmserver.h>
#include <config-workspace.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>

#include <cassert>
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>

#include <QAction>
#include <QApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QFile>
#include <QPushButton>
#include <QRegularExpression>
#include <QSocketNotifier>
#include <QStandardPaths>

#include <KNotification>
#include <KSharedConfig>
#include <QTemporaryFile>
#include <kactioncollection.h>
#include <kauthorized.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kprocess.h>
#include <kshell.h>

#include <KApplicationTrader>
#include <KIO/CommandLauncherJob>
#include <KIO/DesktopExecParser>
#include <KService>

#include <KScreenLocker/KsldApp>

#include <private/qtx11extras_p.h>

#include <krandom.h>
#include <qstandardpaths.h>
#include <startup_interface.h>

#include "kscreenlocker_interface.h"
#include "kwinsession_interface.h"

#include <KUpdateLaunchEnvironmentJob>

extern "C" {
#include <X11/ICE/ICEmsg.h>
#include <X11/ICE/ICEproto.h>
#include <X11/ICE/ICEutil.h>
}

// ICEmsg.h has a static_assert macro, which conflicts with C++'s static_assert.
#ifdef static_assert
#undef static_assert
#endif

namespace
{
size_t fileNumberLimit()
{
    static auto limit = []() -> size_t {
        struct rlimit limit {
        };
        if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
            const auto err = errno;
            qCWarning(KSMSERVER) << "Failed to getrlimit:" << strerror(err);
            return std::numeric_limits<size_t>::max();
        }
        return limit.rlim_cur;
    }();
    return limit;
}

size_t perAppStartLimit()
{
    static auto limit = fileNumberLimit() / 4;
    return limit;
}
} // namespace

KSMServer *the_server = nullptr;

KSMServer *KSMServer::self()
{
    return the_server;
}

/*! Utility function to execute a command on the local machine. Used
 * to restart applications.
 */
void KSMServer::startApplication(const QStringList &cmd, const QString &clientMachine, const QString &userId)
{
    QStringList command = cmd;
    if (command.isEmpty()) {
        return;
    }
    if (!userId.isEmpty()) {
        struct passwd *pw = getpwuid(getuid());
        if (pw != nullptr && userId != QString::fromLocal8Bit(pw->pw_name)) {
            command.prepend(QStringLiteral("--"));
            command.prepend(userId);
            command.prepend(QStringLiteral("-u"));
            command.prepend(QStandardPaths::findExecutable(QStringLiteral("kdesu")));
        }
    }
    if (!clientMachine.isEmpty() && clientMachine != QLatin1String("localhost")) {
        command.prepend(clientMachine);
        command.prepend(xonCommand); // "xon" by default
    }

    const QString app = command.takeFirst();
    const QStringList argList = command;
    auto *job = new KIO::CommandLauncherJob(app, argList);
    auto apps = KApplicationTrader::query([&app](const KService::Ptr &service) {
        const QString binary = KIO::DesktopExecParser::executablePath(service->exec());
        return !service->noDisplay() && !binary.isEmpty() && app.endsWith(binary);
    });
    if (!apps.empty()) {
        job->setDesktopName(apps[0]->desktopEntryName());
    }
    job->start();
}

/*! Utility function to execute a command on the local machine. Used
 * to discard session data
 */
void KSMServer::executeCommand(const QStringList &command)
{
    if (command.isEmpty()) {
        return;
    }

    KProcess::execute(command);
}

IceAuthDataEntry *authDataEntries = nullptr;

static QTemporaryFile *remTempFile = nullptr;

static IceListenObj *listenObjs = nullptr;
int numTransports = 0;
static bool only_local = false;

static Bool HostBasedAuthProc(char * /*hostname*/)
{
    return only_local;
}

Status KSMRegisterClientProc(SmsConn /* smsConn */, SmPointer managerData, char *previousId)
{
    KSMClient *client = (KSMClient *)managerData;
    client->registerClient(previousId);
    return 1;
}

void KSMInteractRequestProc(SmsConn /* smsConn */, SmPointer managerData, int dialogType)
{
    the_server->interactRequest((KSMClient *)managerData, dialogType);
}

void KSMInteractDoneProc(SmsConn /* smsConn */, SmPointer managerData, Bool cancelShutdown)
{
    the_server->interactDone((KSMClient *)managerData, cancelShutdown);
}

void KSMSaveYourselfRequestProc(SmsConn smsConn, SmPointer /* managerData */, int saveType, Bool shutdown, int interactStyle, Bool fast, Bool global)
{
    if (shutdown) {
        qCWarning(KSMSERVER) << "Shutdown via ICE was called but not supported";
    }
    if (!global) {
        SmsSaveYourself(smsConn, saveType, false, interactStyle, fast);
        SmsSaveComplete(smsConn);
    }
    // else checkpoint only, ksmserver does not yet support this
    // mode. Will come for KDE 3.1
}

void KSMSaveYourselfPhase2RequestProc(SmsConn /* smsConn */, SmPointer managerData)
{
    the_server->phase2Request((KSMClient *)managerData);
}

void KSMSaveYourselfDoneProc(SmsConn /* smsConn */, SmPointer managerData, Bool success)
{
    the_server->saveYourselfDone((KSMClient *)managerData, success);
}

void KSMCloseConnectionProc(SmsConn smsConn, SmPointer managerData, int count, char **reasonMsgs)
{
    the_server->deleteClient((KSMClient *)managerData);
    if (count) {
        SmFreeReasons(count, reasonMsgs);
    }
    IceConn iceConn = SmsGetIceConnection(smsConn);
    SmsCleanUp(smsConn);
    IceSetShutdownNegotiation(iceConn, false);
    IceCloseConnection(iceConn);
}

void KSMSetPropertiesProc(SmsConn /* smsConn */, SmPointer managerData, int numProps, SmProp **props)
{
    auto client = (KSMClient *)managerData;
    for (int i = 0; i < numProps; i++) {
        SmProp *p = client->property(props[i]->name);
        if (p) {
            client->properties.removeAll(p);
            SmFreeProperty(p);
        }
        client->properties.append(props[i]);
    }

    if (numProps) {
        free(props);
    }
}

void KSMDeletePropertiesProc(SmsConn /* smsConn */, SmPointer managerData, int numProps, char **propNames)
{
    auto client = (KSMClient *)managerData;
    for (int i = 0; i < numProps; i++) {
        SmProp *p = client->property(propNames[i]);
        if (p) {
            client->properties.removeAll(p);
            SmFreeProperty(p);
        }
    }
}

void KSMGetPropertiesProc(SmsConn smsConn, SmPointer managerData)
{
    auto client = (KSMClient *)managerData;
    auto props = new SmProp *[client->properties.count()];
    int i = 0;
    foreach (SmProp *prop, client->properties)
        props[i++] = prop;

    SmsReturnProperties(smsConn, i, props);
    delete[] props;
}

class KSMListener : public QSocketNotifier
{
public:
    explicit KSMListener(IceListenObj obj)
        : QSocketNotifier(IceGetListenConnectionNumber(obj), QSocketNotifier::Read)
        , listenObj(obj)
    {
    }

    IceListenObj listenObj;
};

class KSMConnection : public QSocketNotifier
{
public:
    explicit KSMConnection(IceConn conn)
        : QSocketNotifier(IceConnectionNumber(conn), QSocketNotifier::Read)
        , iceConn(conn)
    {
        count++;
    }

    ~KSMConnection() override
    {
        count--;
    }

    Q_DISABLE_COPY_MOVE(KSMConnection)

    IceConn iceConn;
    inline static size_t count = 0;
};

/* for printing hex digits */
static void fprintfhex(FILE *fp, unsigned int len, char *cp)
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
static void write_iceauth(FILE *addfp, FILE *removefp, IceAuthDataEntry *entry)
{
    fprintf(addfp, "add %s \"\" %s %s ", entry->protocol_name, entry->network_id, entry->auth_name);
    fprintfhex(addfp, entry->auth_data_length, entry->auth_data);
    fprintf(addfp, "\n");

    fprintf(removefp, "remove protoname=%s protodata=\"\" netid=%s authname=%s\n", entry->protocol_name, entry->network_id, entry->auth_name);
}

#define MAGIC_COOKIE_LEN 16

Status SetAuthentication_local(int count, IceListenObj *listenObjs)
{
    for (int i = 0; i < count; i++) {
        char *prot = IceGetListenConnectionString(listenObjs[i]);
        if (!prot) {
            continue;
        }
        char *host = strchr(prot, '/');
        char *sock = nullptr;
        if (host) {
            *host = 0;
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
        IceSetHostBasedAuthProc(listenObjs[i], HostBasedAuthProc);
        free(prot);
    }
    return 1;
}

Status SetAuthentication(int count, IceListenObj *listenObjs, IceAuthDataEntry **authDataEntries)
{
    QTemporaryFile addTempFile;
    remTempFile = new QTemporaryFile;

    if (!addTempFile.open() || !remTempFile->open()) {
        return 0;
    }

    if ((*authDataEntries = (IceAuthDataEntry *)malloc(count * 2 * sizeof(IceAuthDataEntry))) == nullptr) {
        return 0;
    }

    FILE *addAuthFile = fopen(QFile::encodeName(addTempFile.fileName()).constData(), "r+");
    FILE *remAuthFile = fopen(QFile::encodeName(remTempFile->fileName()).constData(), "r+");

    for (int i = 0; i < numTransports * 2; i += 2) {
        (*authDataEntries)[i].network_id = IceGetListenConnectionString(listenObjs[i / 2]);
        (*authDataEntries)[i].protocol_name = (char *)"ICE";
        (*authDataEntries)[i].auth_name = (char *)"MIT-MAGIC-COOKIE-1";

        (*authDataEntries)[i].auth_data = IceGenerateMagicCookie(MAGIC_COOKIE_LEN);
        (*authDataEntries)[i].auth_data_length = MAGIC_COOKIE_LEN;

        (*authDataEntries)[i + 1].network_id = IceGetListenConnectionString(listenObjs[i / 2]);
        (*authDataEntries)[i + 1].protocol_name = (char *)"XSMP";
        (*authDataEntries)[i + 1].auth_name = (char *)"MIT-MAGIC-COOKIE-1";

        (*authDataEntries)[i + 1].auth_data = IceGenerateMagicCookie(MAGIC_COOKIE_LEN);
        (*authDataEntries)[i + 1].auth_data_length = MAGIC_COOKIE_LEN;

        write_iceauth(addAuthFile, remAuthFile, &(*authDataEntries)[i]);
        write_iceauth(addAuthFile, remAuthFile, &(*authDataEntries)[i + 1]);

        IceSetPaAuthData(2, &(*authDataEntries)[i]);

        IceSetHostBasedAuthProc(listenObjs[i / 2], HostBasedAuthProc);
    }
    fclose(addAuthFile);
    fclose(remAuthFile);

    QString iceAuth = QStandardPaths::findExecutable(QStringLiteral("iceauth"));
    if (iceAuth.isEmpty()) {
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
    if (only_local) {
        return;
    }

    for (int i = 0; i < count * 2; i++) {
        free(authDataEntries[i].network_id);
        free(authDataEntries[i].auth_data);
    }

    free(authDataEntries);

    QString iceAuth = QStandardPaths::findExecutable(QStringLiteral("iceauth"));
    if (iceAuth.isEmpty()) {
        qCWarning(KSMSERVER, "KSMServer: could not find iceauth");
        return;
    }

    if (remTempFile) {
        KProcess p;
        p << iceAuth << QStringLiteral("source") << remTempFile->fileName();
        p.execute();
    }

    delete remTempFile;
    remTempFile = nullptr;
}

static int Xio_ErrorHandler(Display *)
{
    qCWarning(KSMSERVER, "ksmserver: Fatal IO error: client killed");

    // Don't do anything that might require the X connection
    if (the_server) {
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
    std::ignore = ::write(wake_up_socket, &ch, 1);
}

void KSMWatchProc(IceConn iceConn, IcePointer client_data, Bool opening, IcePointer *watch_data)
{
    auto ds = (KSMServer *)client_data;

    if (opening) {
        *watch_data = (IcePointer)ds->watchConnection(iceConn);
    } else {
        ds->removeConnection((KSMConnection *)*watch_data);
    }
}

static Status KSMNewClientProc(SmsConn conn, SmPointer manager_data, unsigned long *mask_ret, SmsCallbacks *cb, char **failure_reason_ret)
{
    *failure_reason_ret = nullptr;

    void *client = ((KSMServer *)manager_data)->newClient(conn);
    if (!client) {
        const char *errstr = "Connection rejected: ksmserver is shutting down";
        qCWarning(KSMSERVER, "%s", errstr);

        if ((*failure_reason_ret = (char *)malloc(strlen(errstr) + 1)) != nullptr) {
            strcpy(*failure_reason_ret, errstr);
        }
        return 0;
    }

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

    *mask_ret = SmsRegisterClientProcMask | SmsInteractRequestProcMask | SmsInteractDoneProcMask | SmsSaveYourselfRequestProcMask
        | SmsSaveYourselfP2RequestProcMask | SmsSaveYourselfDoneProcMask | SmsCloseConnectionProcMask | SmsSetPropertiesProcMask | SmsDeletePropertiesProcMask
        | SmsGetPropertiesProcMask;
    return 1;
}

#ifdef HAVE__ICETRANSNOLISTEN
extern "C" int _IceTransNoListen(const char *protocol);
#endif

KSMServer::KSMServer(InitFlags flags)
    : sessionGroup(QLatin1String(""))
    , m_kwinInterface(new OrgKdeKWinSessionInterface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Session"), QDBusConnection::sessionBus(), this))
    , sockets{-1, -1}
{
    if (!flags.testFlag(InitFlag::NoLockScreen)) {
        ScreenLocker::KSldApp::self()->initialize();
        if (flags.testFlag(InitFlag::ImmediateLockScreen)) {
            ScreenLocker::KSldApp::self()->lock(ScreenLocker::EstablishLock::Immediate);
        }
    }

    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) != 0) {
        qFatal("Could not create socket pair, error %d (%s)", errno, strerror(errno));
    }
    wake_up_socket = sockets[0];
    auto notifier = new QSocketNotifier(sockets[1], QSocketNotifier::Read, this);
    qApp->connect(notifier, &QSocketNotifier::activated, &QApplication::quit);

    new KSMServerInterfaceAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/KSMServer"), this);
    the_server = this;
    clean = false;

    state = Idle;
    saveSession = false;
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("General"));
    clientInteracting = nullptr;
    xonCommand = config.readEntry("xonCommand", "xon");

    only_local = flags.testFlag(InitFlag::OnlyLocal);
#ifdef HAVE__ICETRANSNOLISTEN
    if (only_local) {
        _IceTransNoListen("tcp");
    }
#else
    only_local = false;
#endif

    char errormsg[256];
    if (!SmsInitialize((char *)KSMVendorString, (char *)KSMReleaseString, KSMNewClientProc, (SmPointer)this, HostBasedAuthProc, 256, errormsg)) {
        qCWarning(KSMSERVER, "KSMServer: could not register XSM protocol");
    }

    if (!IceListenForConnections(&numTransports, &listenObjs, 256, errormsg)) {
        qCWarning(KSMSERVER, "KSMServer: Error listening for connections: %s", errormsg);
        qCWarning(KSMSERVER, "KSMServer: Aborting.");
        exit(1);
    }

    {
        // publish available transports.
        QByteArray fName =
            QFile::encodeName(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + QDir::separator() + QStringLiteral("KSMserver"));
        qCDebug(KSMSERVER) << fName;
        QString display = QString::fromLocal8Bit(::getenv("DISPLAY"));
        // strip the screen number from the display
        display.remove(QRegularExpression(QStringLiteral("\\.[0-9]+$")));
        int i = 0;
        while ((i = display.indexOf(QLatin1Char(':'))) >= 0) {
            display[i] = QLatin1Char('_');
        }
        while ((i = display.indexOf(QLatin1Char('/'))) >= 0) {
            display[i] = QLatin1Char('_');
        }

        fName += '_' + display.toLocal8Bit();
        FILE *f = ::fopen(fName.data(), "w+");
        if (!f) {
            qCWarning(KSMSERVER, "KSMServer: cannot open %s: %s", fName.data(), strerror(errno));
            qCWarning(KSMSERVER, "KSMServer: Aborting.");
            exit(1);
        }
        char *session_manager = IceComposeNetworkIdList(numTransports, listenObjs);
        fprintf(f, "%s\n%i\n", session_manager, getpid());
        fclose(f);
        setenv("SESSION_MANAGER", session_manager, true);

        QProcessEnvironment newEnv;
        newEnv.insert(QStringLiteral("SESSION_MANAGER"), QString::fromLatin1(session_manager));
        auto updateEnvJob = new KUpdateLaunchEnvironmentJob(newEnv);
        QEventLoop loop;
        QObject::connect(updateEnvJob, &KUpdateLaunchEnvironmentJob::finished, &loop, &QEventLoop::quit);
        loop.exec();

        free(session_manager);
    }

    if (only_local) {
        if (!SetAuthentication_local(numTransports, listenObjs)) {
            qFatal("KSMSERVER: authentication setup failed.");
        }
    } else {
        if (!SetAuthentication(numTransports, listenObjs, &authDataEntries)) {
            qFatal("KSMSERVER: authentication setup failed.");
        }
    }

    IceAddConnectionWatch(KSMWatchProc, (IcePointer)this);

    KSMListener *con = nullptr;
    for (int i = 0; i < numTransports; i++) {
        fcntl(IceGetListenConnectionNumber(listenObjs[i]), F_SETFD, FD_CLOEXEC);
        con = new KSMListener(listenObjs[i]);
        listener.append(con);
        connect(con, &KSMListener::activated, this, &KSMServer::newConnection);
    }

    signal(SIGHUP, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGPIPE, SIG_IGN);

    connect(&protectionTimer, &QTimer::timeout, this, &KSMServer::protectionTimeout);
    connect(this, &KSMServer::sessionRestored, this, [this]() {
        auto reply = m_restoreSessionCall.createReply();
        QDBusConnection::sessionBus().send(reply);
        m_restoreSessionCall = QDBusMessage();
    });
    connect(qApp, &QApplication::aboutToQuit, this, &KSMServer::cleanUp);

    setupXIOErrorHandler();

    QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                                                         QStringLiteral("/KSplash"),
                                                                         QStringLiteral("org.kde.KSplash"),
                                                                         QStringLiteral("setStage"));
    ksplashProgressMessage.setArguments({QStringLiteral("ksmserver")});
    QDBusConnection::sessionBus().call(ksplashProgressMessage, QDBus::NoBlock);
}

KSMServer::~KSMServer()
{
    qDeleteAll(listener);
    the_server = nullptr;
    cleanUp();
}

void KSMServer::cleanUp()
{
    if (clean) {
        return;
    }
    clean = true;
    IceFreeListenObjs(numTransports, listenObjs);

    wake_up_socket = -1;
    ::close(sockets[1]);
    ::close(sockets[0]);
    sockets[0] = -1;
    sockets[1] = -1;

    QByteArray fName = QFile::encodeName(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + QLatin1Char('/') + QStringLiteral("KSMserver"));
    QString display = QString::fromLocal8Bit(::getenv("DISPLAY"));
    // strip the screen number from the display
    display.remove(QRegularExpression(QStringLiteral("\\.[0-9]+$")));
    int i = 0;
    while ((i = display.indexOf(QLatin1Char(':'))) >= 0) {
        display[i] = QLatin1Char('_');
    }
    while ((i = display.indexOf(QLatin1Char('/'))) >= 0) {
        display[i] = QLatin1Char('_');
    }

    fName += '_' + display.toLocal8Bit();
    ::unlink(fName.data());

    FreeAuthenticationData(numTransports, authDataEntries);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
}

void *KSMServer::watchConnection(IceConn iceConn)
{
    auto conn = new KSMConnection(iceConn);
    connect(conn, &KSMConnection::activated, this, &KSMServer::processData);
    return (void *)conn;
}

void KSMServer::removeConnection(KSMConnection *conn)
{
    delete conn;
}

/*!
  Called from our IceIoErrorHandler
 */
void KSMServer::ioError(IceConn /*iceConn*/)
{
}

void KSMServer::processData(int /*socket*/)
{
    auto iceConn = ((KSMConnection *)sender())->iceConn;
    IceProcessMessagesStatus status = IceProcessMessages(iceConn, nullptr, nullptr);
    if (status == IceProcessMessagesIOError) {
        IceSetShutdownNegotiation(iceConn, false);
        QList<KSMClient *>::iterator it = clients.begin();
        QList<KSMClient *>::iterator const itEnd = clients.end();
        while ((it != itEnd) && *it && (SmsGetIceConnection((*it)->connection()) != iceConn)) {
            ++it;
        }
        if ((it != itEnd) && *it) {
            SmsConn smsConn = (*it)->connection();
            deleteClient(*it);
            SmsCleanUp(smsConn);
        }
        (void)IceCloseConnection(iceConn);
    }
}

KSMClient *KSMServer::newClient(SmsConn conn)
{
    KSMClient *client = nullptr;
    if (state != Killing) {
        client = new KSMClient(conn);
        clients.append(client);
    }
    return client;
}

void KSMServer::deleteClient(KSMClient *client)
{
    if (!clients.contains(client)) { // paranoia
        return;
    }
    clients.removeAll(client);
    clientsToKill.removeAll(client);
    clientsToSave.removeAll(client);
    if (client == clientInteracting) {
        clientInteracting = nullptr;
        handlePendingInteractions();
    }
    delete client;
    if (state == Shutdown || state == Checkpoint || state == ClosingSubSession) {
        completeShutdownOrCheckpoint();
    }
    if (state == Killing) {
        completeKilling();
    } else if (state == KillingSubSession) {
        completeKillingSubSession();
    }
}

void KSMServer::newConnection(int /*socket*/)
{
    IceAcceptStatus status = IceAcceptFailure;
    auto iceConn = IceAcceptConnection(((KSMListener *)sender())->listenObj, &status);
    if (!iceConn) {
        return;
    }

    // This should be sufficient to hold all open file descriptors that are not connection watches. The rest we will
    // freely use to watch ICE connections.
    static const size_t backupFileCount = 128;
    qCDebug(KSMSERVER) << "KSMConnection::count" << KSMConnection::count;
    if (KSMConnection::count > (fileNumberLimit() - backupFileCount)) {
        // https://bugs.kde.org/show_bug.cgi?id=475506
        qCWarning(KSMSERVER) << "Too many open connections. Refusing to track any more to prevent exhaustion of open file limits.";
        QHash<QString, size_t> consumers;
        for (const auto &client : clients) {
            QString program = client->program();
            if (program.isEmpty() && !client->restartCommand().isEmpty()) {
                program = client->restartCommand().first();
            }
            if (program.isEmpty()) {
                program = i18nc("@label an unknown executable is using resources", "[unknown]");
            }

            if (!consumers.contains(program)) {
                consumers[program] = 0;
            }
            consumers[program]++;
        }
        std::multimap<size_t, QString> ranking; // using STL container because QMultiMap has no reverse iterators
        for (auto it = consumers.cbegin(); it != consumers.cend(); it++) {
            ranking.emplace(it.value(), it.key());
        }

        QString consumerInfo;
        int i = 0;
        for (auto it = ranking.rbegin(); i != 3 && it != ranking.rend(); it++, i++) {
            if (!consumerInfo.isEmpty()) {
                consumerInfo += QLatin1String(", ");
            }
            consumerInfo += QStringLiteral("%1:%2").arg(it->second, QString::number(it->first));
        }
        KNotification::event(KNotification::Error,
                             xi18nc("@label notification; %1 is a list of executables",
                                    "Unable to manage some apps because the system's session management resources are exhausted. Here are the top three "
                                    "consumers of session resources:\n%1",
                                    consumerInfo),
                             QPixmap(),
                             KNotification::Persistent);

        std::ignore = IceCloseConnection(iceConn);
        return;
    }

    IceSetShutdownNegotiation(iceConn, false);
    IceConnectStatus cstatus = IceConnectPending;
    while ((cstatus = IceConnectionStatus(iceConn)) == IceConnectPending) {
        (void)IceProcessMessages(iceConn, nullptr, nullptr);
    }

    if (cstatus != IceConnectAccepted) {
        if (cstatus == IceConnectIOError) {
            qCDebug(KSMSERVER) << "IO error opening ICE Connection!";
        } else {
            qCDebug(KSMSERVER) << "ICE Connection rejected!";
        }
        std::ignore = IceCloseConnection(iceConn);
        return;
    }

    // don't leak the fd
    fcntl(IceConnectionNumber(iceConn), F_SETFD, FD_CLOEXEC);
}

QString KSMServer::currentSession()
{
    if (sessionGroup.startsWith(SESSION_PREFIX)) {
        return sessionGroup.mid(SESSION_PREFIX.size());
    }
    return QLatin1String(""); // empty, not null, since used for KConfig::setGroup // TODO does this comment make any sense?
}

void KSMServer::discardSession()
{
    KConfigGroup config(KSharedConfig::openConfig(), sessionGroup);
    int count = config.readEntry("count", 0);
    foreach (KSMClient *c, clients) {
        QStringList discardCommand = c->discardCommand();
        if (discardCommand.isEmpty()) {
            continue;
        }
        // check that non of the old clients used the exactly same
        // discardCommand before we execute it. This used to be the
        // case up to KDE and Qt < 3.1
        int i = 1;
        while (i <= count && config.readPathEntry(QStringLiteral("discardCommand") + QString::number(i), QStringList()) != discardCommand) {
            i++;
        }
        if (i <= count) {
            executeCommand(discardCommand);
        }
    }
}

void KSMServer::storeSession()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration(); // config may have changed in the KControl module
    KConfigGroup generalGroup(config, QStringLiteral("General"));
    excludeApps = generalGroup.readEntry("excludeApps").toLower().split(QRegularExpression(QStringLiteral("[,:]")), Qt::SkipEmptyParts);
    KConfigGroup configSessionGroup(config, sessionGroup);
    int count = configSessionGroup.readEntry("count", 0);
    for (int i = 1; i <= count; i++) {
        const auto discardCommand = configSessionGroup.readPathEntry(QLatin1String("discardCommand") + QString::number(i), QStringList());
        if (discardCommand.isEmpty()) {
            continue;
        }
        // check that non of the new clients uses the exactly same
        // discardCommand before we execute it. This used to be the
        // case up to KDE and Qt < 3.1
        QList<KSMClient *>::iterator it = clients.begin();
        QList<KSMClient *>::iterator const itEnd = clients.end();
        while ((it != itEnd) && *it && (discardCommand != (*it)->discardCommand())) {
            ++it;
        }
        if ((it != itEnd) && *it) {
            continue;
        }
        executeCommand(discardCommand);
    }
    config->deleteGroup(sessionGroup); // ### does not work with global config object...
    KConfigGroup cg(config, sessionGroup);
    count = 0;

    // Tell kwin to save its state
    auto reply = m_kwinInterface->finishSaveSession(currentSession());
    reply.waitForFinished(); // boo!

    foreach (KSMClient *c, clients) {
        const int restartHint = c->restartStyleHint();
        if (restartHint == SmRestartNever) {
            continue;
        }
        const QString program = c->program();
        const QStringList restartCommand = c->restartCommand();
        if (program.isEmpty() && restartCommand.isEmpty()) {
            continue;
        }
        if (state == ClosingSubSession && !clientsToSave.contains(c)) {
            continue;
        }

        // 'program' might be (mostly) fullpath, or (sometimes) just the name.
        // 'name' is just the name.
        const QFileInfo info(program);
        const QString &name = info.fileName();

        if (excludeApps.contains(program.toLower()) || excludeApps.contains(name.toLower())) {
            continue;
        }

        count++;
        const QString n = QString::number(count);
        cg.writeEntry(QStringLiteral("program") + n, program);
        cg.writeEntry(QStringLiteral("clientId") + n, c->clientId());
        cg.writeEntry(QStringLiteral("restartCommand") + n, restartCommand);
        cg.writePathEntry(QStringLiteral("discardCommand") + n, c->discardCommand());
        cg.writeEntry(QStringLiteral("restartStyleHint") + n, restartHint);
        cg.writeEntry(QStringLiteral("userId") + n, c->userId());
    }
    cg.writeEntry("count", count);

    KConfigGroup cg2(config, QStringLiteral("General"));

    storeLegacySession(config.data());
    config->sync();
}

QStringList KSMServer::sessionList()
{
    QStringList sessions(QStringLiteral("default"));
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    const QStringList groups = config->groupList();
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        if ((*it).startsWith(SESSION_PREFIX)) {
            sessions << (*it).mid(SESSION_PREFIX.size());
        }
    }
    return sessions;
}

bool KSMServer::defaultSession() const
{
    return sessionGroup.isEmpty();
}

void KSMServer::setRestoreSession(const QString &sessionName)
{
    if (state != Idle) {
        return;
    }
    qCDebug(KSMSERVER) << "KSMServer::restoreSession " << sessionName;
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    sessionGroup = SESSION_PREFIX + sessionName;
    KConfigGroup configSessionGroup(config, sessionGroup);

    int count = configSessionGroup.readEntry("count", 0);
    appsToStart = count;
}

/*!
  Starts the default session.
 */
void KSMServer::startDefaultSession()
{
    if (state != Idle) {
        return;
    }
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

    state = KSMServer::Restoring;

    auto reply = m_kwinInterface->loadSession(currentSession());
    auto watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (reply.isError()) {
            qWarning() << "Failed to notify kwin of current session " << reply.error().message();
        }
        restoreLegacySession(KSharedConfig::openConfig().data());
        tryRestore();
    });
}

void KSMServer::restoreSubSession(const QString &name)
{
    sessionGroup = SUBSESSION_PREFIX + name;

    KConfigGroup configSessionGroup(KSharedConfig::openConfig(), sessionGroup);
    int count = configSessionGroup.readEntry("count", 0);
    appsToStart = count;

    state = RestoringSubSession;
    tryRestore();
}

void KSMServer::clientRegistered(const char * /* previousId */)
{
}

void KSMServer::tryRestore()
{
    if (state != Restoring && state != RestoringSubSession) {
        return;
    }
    KConfigGroup config(KSharedConfig::openConfig(), sessionGroup);

    struct DontStartEntry {
        QString clientId;
        QString appName;
    };
    QList<DontStartEntry> dontStartEntries;
    QHash<QString, size_t> startCounter;

    struct RestartEntry {
        QString clientId;
        QStringList restartCommand;
        int restartStyleHint;
        QString clientMachine;
        QString userId;
    };
    QList<RestartEntry> entries;
    entries.reserve(appsToStart);
    for (int i = 0; i < appsToStart; i++) {
        const auto n = QString::number(i);
        const auto entry = entries.emplace_back(RestartEntry{
            .clientId = config.readEntry(QLatin1String("clientId") + n, QString()),
            .restartCommand = config.readEntry(QLatin1String("restartCommand") + n, QStringList()),
            .restartStyleHint = config.readEntry(QLatin1String("restartStyleHint") + n, 0),
            .clientMachine = config.readEntry(QLatin1String("clientMachine") + n, QString()),
            .userId = config.readEntry(QLatin1String("userId") + n, QString()),
        });

        // Count how many times this command is going to be started. If it is too many we will create a DontStartEntry
        // and consequently turn *all* restorations no-op in the actual start loop.
        if (entry.restartCommand.isEmpty()) {
            continue;
        }
        const auto appName = entry.restartCommand.first();
        if (!startCounter.contains(appName)) {
            startCounter[appName] = 0;
        }
        startCounter[appName]++;
        if (startCounter[appName] == perAppStartLimit()) { // only insert this entry once! when the limit is hit
            dontStartEntries.push_back(DontStartEntry{.clientId = entry.clientId, .appName = appName});
        }
    }

    for (const auto &entry : dontStartEntries) {
        qCWarning(KSMSERVER) << "Too many application starts issued for" << entry.appName << ". Not starting any more."
                             << "Something may be broken with the application's session management.";
        KNotification::event(KNotification::Error,
                             xi18nc("@label notification; %1 is an executable name",
                                    "Unable to restore <application>%1</application> because it is broken and has exhausted the system's session restoration "
                                    "resources. Please report this to the app's developers.",
                                    entry.appName));
    }

    for (const auto &entry : entries) {
        // We only discard the entries here because a violating app will get all entries disabled, not just the ones in
        // excess. So we need to loop all entries twice: once to establish the in-excess apps, and again to actually start
        // (or not).
        const bool dontStart = std::any_of(dontStartEntries.cbegin(), dontStartEntries.cend(), [&entry](const auto &dontStartEntry) {
            return dontStartEntry.clientId == entry.clientId;
        });
        if (dontStart) {
            continue;
        }

        const bool alreadyStarted = std::any_of(clients.cbegin(), clients.cend(), [&entry](const auto &client) {
            return QString::fromLocal8Bit(client->clientId()) == entry.clientId;
        });
        if (alreadyStarted) {
            continue;
        }

        if (entry.restartCommand.isEmpty() || entry.restartStyleHint == SmRestartNever) {
            continue;
        }
        startApplication(entry.restartCommand, entry.clientMachine, entry.userId);
    }

    // all done
    if (state == Restoring) {
        Q_EMIT sessionRestored();
    } else { // subsession
        Q_EMIT subSessionOpened();
    }
    state = Idle;
}

void KSMServer::startupDone()
{
    state = Idle;
}

void KSMServer::openSwitchUserDialog()
{
    // this method exists only for compatibility. Users should ideally call this directly
    OrgKdeScreensaverInterface iface(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus());
    iface.SwitchUser();
}
