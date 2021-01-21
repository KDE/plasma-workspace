/*
   Copyright (C) 2004 Oswald Buddenhagen <ossi@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kdisplaymanager.h"

#if HAVE_X11

#include <kuser.h>

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QX11Info>

#include <X11/Xauth.h>
#include <X11/Xlib.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define _DBUS_PROPERTIES_IFACE "org.freedesktop.DBus.Properties"
#define _DBUS_PROPERTIES_GET "Get"

#define DBUS_PROPERTIES_IFACE QLatin1String(_DBUS_PROPERTIES_IFACE)
#define DBUS_PROPERTIES_GET QLatin1String(_DBUS_PROPERTIES_GET)

#define _SYSTEMD_SERVICE "org.freedesktop.login1"
#define _SYSTEMD_BASE_PATH "/org/freedesktop/login1"
#define _SYSTEMD_MANAGER_IFACE _SYSTEMD_SERVICE ".Manager"
#define _SYSTEMD_SESSION_BASE_PATH _SYSTEMD_BASE_PATH "/session"
#define _SYSTEMD_SEAT_IFACE _SYSTEMD_SERVICE ".Seat"
#define _SYSTEMD_SEAT_BASE_PATH _SYSTEMD_BASE_PATH "/seat"
#define _SYSTEMD_SESSION_IFACE _SYSTEMD_SERVICE ".Session"
#define _SYSTEMD_USER_PROPERTY "User"
#define _SYSTEMD_SEAT_PROPERTY "Seat"
#define _SYSTEMD_SESSIONS_PROPERTY "Sessions"
#define _SYSTEMD_SWITCH_PROPERTY "Activate"

#define SYSTEMD_SERVICE QLatin1String(_SYSTEMD_SERVICE)
#define SYSTEMD_BASE_PATH QLatin1String(_SYSTEMD_BASE_PATH)
#define SYSTEMD_MANAGER_IFACE QLatin1String(_SYSTEMD_MANAGER_IFACE)
#define SYSTEMD_SESSION_BASE_PATH QLatin1String(_SYSTEMD_SESSION_BASE_PATH)
#define SYSTEMD_SEAT_IFACE QLatin1String(_SYSTEMD_SEAT_IFACE)
#define SYSTEMD_SEAT_BASE_PATH QLatin1String(_SYSTEMD_SEAT_BASE_PATH)
#define SYSTEMD_SESSION_IFACE QLatin1String(_SYSTEMD_SESSION_IFACE)
#define SYSTEMD_USER_PROPERTY QLatin1String(_SYSTEMD_USER_PROPERTY)
#define SYSTEMD_SEAT_PROPERTY QLatin1String(_SYSTEMD_SEAT_PROPERTY)
#define SYSTEMD_SESSIONS_PROPERTY QLatin1String(_SYSTEMD_SESSIONS_PROPERTY)
#define SYSTEMD_SWITCH_CALL QLatin1String(_SYSTEMD_SWITCH_PROPERTY)

struct NamedDBusObjectPath {
    QString name;
    QDBusObjectPath path;
};
Q_DECLARE_METATYPE(NamedDBusObjectPath)
Q_DECLARE_METATYPE(QList<NamedDBusObjectPath>)

// Marshall the NamedDBusObjectPath data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const NamedDBusObjectPath &namedPath)
{
    argument.beginStructure();
    argument << namedPath.name << namedPath.path;
    argument.endStructure();
    return argument;
}

// Retrieve the NamedDBusObjectPath data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, NamedDBusObjectPath &namedPath)
{
    argument.beginStructure();
    argument >> namedPath.name >> namedPath.path;
    argument.endStructure();
    return argument;
}

struct NumberedDBusObjectPath {
    uint num;
    QDBusObjectPath path;
};
Q_DECLARE_METATYPE(NumberedDBusObjectPath)

// Marshall the NumberedDBusObjectPath data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const NumberedDBusObjectPath &numberedPath)
{
    argument.beginStructure();
    argument << numberedPath.num << numberedPath.path;
    argument.endStructure();
    return argument;
}

// Retrieve the NumberedDBusObjectPath data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, NumberedDBusObjectPath &numberedPath)
{
    argument.beginStructure();
    argument >> numberedPath.num >> numberedPath.path;
    argument.endStructure();
    return argument;
}

class SystemdManager : public QDBusInterface
{
public:
    SystemdManager()
        : QDBusInterface(SYSTEMD_SERVICE, SYSTEMD_BASE_PATH, SYSTEMD_MANAGER_IFACE, QDBusConnection::systemBus())
    {
    }
};

class SystemdSeat : public QDBusInterface
{
public:
    SystemdSeat(const QDBusObjectPath &path)
        : QDBusInterface(SYSTEMD_SERVICE, path.path(), SYSTEMD_SEAT_IFACE, QDBusConnection::systemBus())
    {
    }
    /* HACK to be able to extract a(so) type from QDBus, property doesn't do the trick */
    QList<NamedDBusObjectPath> getSessions()
    {
        QDBusMessage message = QDBusMessage::createMethodCall(service(), path(), DBUS_PROPERTIES_IFACE, DBUS_PROPERTIES_GET);
        message << interface() << SYSTEMD_SESSIONS_PROPERTY;
        QDBusMessage reply = QDBusConnection::systemBus().call(message);

        QVariantList args = reply.arguments();
        if (!args.isEmpty()) {
            QList<NamedDBusObjectPath> namedPathList =
                qdbus_cast<QList<NamedDBusObjectPath>>(args.at(0).value<QDBusVariant>().variant().value<QDBusArgument>());
            return namedPathList;
        }
        return QList<NamedDBusObjectPath>();
    }
};

class SystemdSession : public QDBusInterface
{
public:
    SystemdSession(const QDBusObjectPath &path)
        : QDBusInterface(SYSTEMD_SERVICE, path.path(), SYSTEMD_SESSION_IFACE, QDBusConnection::systemBus())
    {
    }
    /* HACK to be able to extract (so) type from QDBus, property doesn't do the trick */
    NamedDBusObjectPath getSeat()
    {
        QDBusMessage message = QDBusMessage::createMethodCall(service(), path(), DBUS_PROPERTIES_IFACE, DBUS_PROPERTIES_GET);
        message << interface() << SYSTEMD_SEAT_PROPERTY;
        QDBusMessage reply = QDBusConnection::systemBus().call(message);

        QVariantList args = reply.arguments();
        if (!args.isEmpty()) {
            NamedDBusObjectPath namedPath;
            args.at(0).value<QDBusVariant>().variant().value<QDBusArgument>() >> namedPath;
            return namedPath;
        }
        return NamedDBusObjectPath();
    }
    NumberedDBusObjectPath getUser()
    {
        QDBusMessage message = QDBusMessage::createMethodCall(service(), path(), DBUS_PROPERTIES_IFACE, DBUS_PROPERTIES_GET);
        message << interface() << SYSTEMD_USER_PROPERTY;
        QDBusMessage reply = QDBusConnection::systemBus().call(message);

        QVariantList args = reply.arguments();
        if (!args.isEmpty()) {
            NumberedDBusObjectPath numberedPath;
            args.at(0).value<QDBusVariant>().variant().value<QDBusArgument>() >> numberedPath;
            return numberedPath;
        }
        return NumberedDBusObjectPath();
    }
    void getSessionLocation(SessEnt &se)
    {
        se.tty = (property("Type").toString() != QLatin1String("x11"));
        se.display = property(se.tty ? "TTY" : "Display").toString();
        se.vt = property("VTNr").toInt();
    }
};

class CKManager : public QDBusInterface
{
public:
    CKManager()
        : QDBusInterface(QStringLiteral("org.freedesktop.ConsoleKit"),
                         QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
                         QStringLiteral("org.freedesktop.ConsoleKit.Manager"),
                         QDBusConnection::systemBus())
    {
    }
};

class CKSeat : public QDBusInterface
{
public:
    CKSeat(const QDBusObjectPath &path)
        : QDBusInterface(QStringLiteral("org.freedesktop.ConsoleKit"),
                         path.path(),
                         QStringLiteral("org.freedesktop.ConsoleKit.Seat"),
                         QDBusConnection::systemBus())
    {
    }
};

class CKSession : public QDBusInterface
{
public:
    CKSession(const QDBusObjectPath &path)
        : QDBusInterface(QStringLiteral("org.freedesktop.ConsoleKit"),
                         path.path(),
                         QStringLiteral("org.freedesktop.ConsoleKit.Session"),
                         QDBusConnection::systemBus())
    {
    }
    void getSessionLocation(SessEnt &se)
    {
        QString tty;
        QDBusReply<QString> r = call(QStringLiteral("GetX11Display"));
        if (r.isValid() && !r.value().isEmpty()) {
            QDBusReply<QString> r2 = call(QStringLiteral("GetX11DisplayDevice"));
            tty = r2.value();
            se.display = r.value();
            se.tty = false;
        } else {
            QDBusReply<QString> r2 = call(QStringLiteral("GetDisplayDevice"));
            tty = r2.value();
            se.display = tty;
            se.tty = true;
        }
        se.vt = tty.midRef(strlen("/dev/tty")).toInt();
    }
};

class GDMFactory : public QDBusInterface
{
public:
    GDMFactory()
        : QDBusInterface(QStringLiteral("org.gnome.DisplayManager"),
                         QStringLiteral("/org/gnome/DisplayManager/LocalDisplayFactory"),
                         QStringLiteral("org.gnome.DisplayManager.LocalDisplayFactory"),
                         QDBusConnection::systemBus())
    {
    }
};

class LightDMDBus : public QDBusInterface
{
public:
    LightDMDBus()
        : QDBusInterface(QStringLiteral("org.freedesktop.DisplayManager"),
                         qgetenv("XDG_SEAT_PATH"),
                         QStringLiteral("org.freedesktop.DisplayManager.Seat"),
                         QDBusConnection::systemBus())
    {
    }
};

static enum {
    Dunno,
    NoDM,
    NewKDM,
    OldKDM,
    NewGDM,
    OldGDM,
    LightDM,
} DMType = Dunno;
static const char *ctl, *dpy;

class KDisplayManager::Private
{
public:
    Private()
        : fd(-1)
    {
    }
    ~Private()
    {
        if (fd >= 0)
            close(fd);
    }

    int fd;
};

KDisplayManager::KDisplayManager()
    : d(new Private)
{
    const char *ptr;
    struct sockaddr_un sa;

    qDBusRegisterMetaType<NamedDBusObjectPath>();
    qDBusRegisterMetaType<QList<NamedDBusObjectPath>>();
    qDBusRegisterMetaType<NumberedDBusObjectPath>();

    if (DMType == Dunno) {
        dpy = ::getenv("DISPLAY");
        if (dpy && (ctl = ::getenv("DM_CONTROL")))
            DMType = NewKDM;
        else if (dpy && (ctl = ::getenv("XDM_MANAGED")) && ctl[0] == '/')
            DMType = OldKDM;
        else if (::getenv("XDG_SEAT_PATH") && LightDMDBus().isValid())
            DMType = LightDM;
        else if (::getenv("GDMSESSION"))
            DMType = GDMFactory().isValid() ? NewGDM : OldGDM;
        else
            DMType = NoDM;
    }
    switch (DMType) {
    default:
        return;
    case NewKDM:
    case OldGDM:
        if ((d->fd = ::socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
            return;
        sa.sun_family = AF_UNIX;
        if (DMType == OldGDM) {
            strcpy(sa.sun_path, "/var/run/gdm_socket");
            if (::connect(d->fd, (struct sockaddr *)&sa, sizeof(sa))) {
                strcpy(sa.sun_path, "/tmp/.gdm_socket");
                if (::connect(d->fd, (struct sockaddr *)&sa, sizeof(sa))) {
                    ::close(d->fd);
                    d->fd = -1;
                    break;
                }
            }
            GDMAuthenticate();
        } else {
            if ((ptr = strchr(dpy, ':')))
                ptr = strchr(ptr, '.');
            snprintf(sa.sun_path, sizeof(sa.sun_path), "%s/dmctl-%.*s/socket", ctl, ptr ? int(ptr - dpy) : 512, dpy);
            if (::connect(d->fd, (struct sockaddr *)&sa, sizeof(sa))) {
                ::close(d->fd);
                d->fd = -1;
            }
        }
        break;
    case OldKDM: {
        QString tf(ctl);
        tf.truncate(tf.indexOf(','));
        d->fd = ::open(tf.toLatin1(), O_WRONLY);
    } break;
    }
}

KDisplayManager::~KDisplayManager()
{
    delete d;
}

bool KDisplayManager::exec(const char *cmd)
{
    QByteArray buf;

    return exec(cmd, buf);
}

/**
 * Execute a KDM/GDM remote control command.
 * @param cmd the command to execute. FIXME: undocumented yet.
 * @param buf the result buffer.
 * @return result:
 *  @li If true, the command was successfully executed.
 *   @p ret might contain additional results.
 *  @li If false and @p ret is empty, a communication error occurred
 *   (most probably KDM is not running).
 *  @li If false and @p ret is non-empty, it contains the error message
 *   from KDM.
 */
bool KDisplayManager::exec(const char *cmd, QByteArray &buf)
{
    bool ret = false;
    int tl;
    int len = 0;

    if (d->fd < 0)
        goto busted;

    tl = strlen(cmd);
    if (::write(d->fd, cmd, tl) != tl) {
    bust:
        ::close(d->fd);
        d->fd = -1;
    busted:
        buf.resize(0);
        return false;
    }
    if (DMType == OldKDM) {
        buf.resize(0);
        return true;
    }
    for (;;) {
        if (buf.size() < 128)
            buf.resize(128);
        else if (buf.size() < len * 2)
            buf.resize(len * 2);
        if ((tl = ::read(d->fd, buf.data() + len, buf.size() - len)) <= 0) {
            if (tl < 0 && errno == EINTR)
                continue;
            goto bust;
        }
        len += tl;
        if (buf[len - 1] == '\n') {
            buf[len - 1] = 0;
            if (len > 2 && (buf[0] == 'o' || buf[0] == 'O') && (buf[1] == 'k' || buf[1] == 'K') && buf[2] <= ' ')
                ret = true;
            break;
        }
    }
    return ret;
}

static bool getCurrentSeat(QDBusObjectPath *currentSession, QDBusObjectPath *currentSeat)
{
    SystemdManager man;
    if (man.isValid()) {
        *currentSeat = QDBusObjectPath(_SYSTEMD_SEAT_BASE_PATH "/auto");
        SystemdSeat seat(*currentSeat);
        if (seat.property("Id").isValid()) { // query an arbitrary property to confirm the path is valid
            return true;
        }

        // auto is newer and may not exist on all platforms, fallback to GetSessionByPID if the above failed

        QDBusReply<QDBusObjectPath> r = man.call(QStringLiteral("GetSessionByPID"), (uint)QCoreApplication::applicationPid());
        if (r.isValid()) {
            SystemdSession sess(r.value());
            if (sess.isValid()) {
                NamedDBusObjectPath namedPath = sess.getSeat();
                *currentSeat = namedPath.path;
                return true;
            }
        }
    } else {
        CKManager man;
        QDBusReply<QDBusObjectPath> r = man.call(QStringLiteral("GetCurrentSession"));
        if (r.isValid()) {
            CKSession sess(r.value());
            if (sess.isValid()) {
                QDBusReply<QDBusObjectPath> r2 = sess.call(QStringLiteral("GetSeatId"));
                if (r2.isValid()) {
                    if (currentSession)
                        *currentSession = r.value();
                    *currentSeat = r2.value();
                    return true;
                }
            }
        }
    }
    return false;
}

static QList<QDBusObjectPath> getSessionsForSeat(const QDBusObjectPath &path)
{
    if (path.path().startsWith(SYSTEMD_BASE_PATH)) { // systemd path incoming
        SystemdSeat seat(path);
        if (seat.isValid()) {
            QList<NamedDBusObjectPath> r = seat.getSessions();
            QList<QDBusObjectPath> result;
            foreach (const NamedDBusObjectPath &namedPath, r)
                result.append(namedPath.path);
            // This pretty much can't contain any other than local sessions as the seat is retrieved from the current session
            return result;
        }
    } else if (path.path().startsWith(QLatin1String("/org/freedesktop/ConsoleKit"))) {
        CKSeat seat(path);
        if (seat.isValid()) {
            QDBusReply<QList<QDBusObjectPath>> r = seat.call(QStringLiteral("GetSessions"));
            if (r.isValid()) {
                // This will contain only local sessions:
                // - this is only ever called when isSwitchable() is true => local seat
                // - remote logins into the machine are assigned to other seats
                return r.value();
            }
        }
    }
    return QList<QDBusObjectPath>();
}

#ifndef KDM_NO_SHUTDOWN
bool KDisplayManager::canShutdown()
{
    if (DMType == NewGDM || DMType == NoDM || DMType == LightDM) {
        QDBusReply<QString> canPowerOff = SystemdManager().call(QStringLiteral("CanPowerOff"));
        if (canPowerOff.isValid())
            return canPowerOff.value() != QLatin1String("no");
        QDBusReply<bool> canStop = CKManager().call(QStringLiteral("CanStop"));
        if (canStop.isValid())
            return canStop.value();
        return false;
    }

    if (DMType == OldKDM)
        return strstr(ctl, ",maysd") != nullptr;

    QByteArray re;

    if (DMType == OldGDM)
        return exec("QUERY_LOGOUT_ACTION\n", re) && re.indexOf("HALT") >= 0;

    return exec("caps\n", re) && re.indexOf("\tshutdown") >= 0;
}

void KDisplayManager::shutdown(KWorkSpace::ShutdownType shutdownType,
                               KWorkSpace::ShutdownMode shutdownMode, /* NOT Default */
                               const QString &bootOption)
{
    if (shutdownType == KWorkSpace::ShutdownTypeNone || shutdownType == KWorkSpace::ShutdownTypeLogout)
        return;

    bool cap_ask;
    if (DMType == NewKDM) {
        QByteArray re;
        cap_ask = exec("caps\n", re) && re.indexOf("\tshutdown ask") >= 0;
    } else {
        if (!bootOption.isEmpty())
            return;

        if (DMType == NewGDM || DMType == NoDM || DMType == LightDM) {
            // systemd supports only 2 modes:
            // * interactive = true: brings up a PolicyKit prompt if other sessions are active
            // * interactive = false: rejects the shutdown if other sessions are active
            // There are no schedule or force modes.
            // We try to map our 4 shutdown modes in the sanest way.
            bool interactive = (shutdownMode == KWorkSpace::ShutdownModeInteractive || shutdownMode == KWorkSpace::ShutdownModeForceNow);
            QDBusReply<QString> check =
                SystemdManager().call(QLatin1String(shutdownType == KWorkSpace::ShutdownTypeReboot ? "Reboot" : "PowerOff"), interactive);
            if (!check.isValid()) {
                // FIXME: entirely ignoring shutdownMode
                CKManager().call(QLatin1String(shutdownType == KWorkSpace::ShutdownTypeReboot ? "Restart" : "Stop"));
                // if even CKManager call fails, there is nothing more to be done
            }
            return;
        }

        cap_ask = false;
    }
    if (!cap_ask && shutdownMode == KWorkSpace::ShutdownModeInteractive)
        shutdownMode = KWorkSpace::ShutdownModeForceNow;

    QByteArray cmd;
    if (DMType == OldGDM) {
        cmd.append(shutdownMode == KWorkSpace::ShutdownModeForceNow ? "SET_LOGOUT_ACTION " : "SET_SAFE_LOGOUT_ACTION ");
        cmd.append(shutdownType == KWorkSpace::ShutdownTypeReboot ? "REBOOT\n" : "HALT\n");
    } else {
        cmd.append("shutdown\t");
        cmd.append(shutdownType == KWorkSpace::ShutdownTypeReboot ? "reboot\t" : "halt\t");
        if (!bootOption.isEmpty())
            cmd.append("=").append(bootOption.toLocal8Bit()).append("\t");
        cmd.append(shutdownMode == KWorkSpace::ShutdownModeInteractive
                       ? "ask\n"
                       : shutdownMode == KWorkSpace::ShutdownModeForceNow ? "forcenow\n"
                                                                          : shutdownMode == KWorkSpace::ShutdownModeTryNow ? "trynow\n" : "schedule\n");
    }
    exec(cmd.data());
}

bool KDisplayManager::bootOptions(QStringList &opts, int &defopt, int &current)
{
    if (DMType != NewKDM)
        return false;

    QByteArray re;
    if (!exec("listbootoptions\n", re))
        return false;

    opts = QString::fromLocal8Bit(re.data()).split('\t', Qt::SkipEmptyParts);
    if (opts.size() < 4)
        return false;

    bool ok;
    defopt = opts[2].toInt(&ok);
    if (!ok)
        return false;
    current = opts[3].toInt(&ok);
    if (!ok)
        return false;

    opts = opts[1].split(' ', Qt::SkipEmptyParts);
    for (QStringList::Iterator it = opts.begin(); it != opts.end(); ++it)
        (*it).replace(QLatin1String("\\s"), QLatin1String(" "));

    return true;
}
#endif // KDM_NO_SHUTDOWN

bool KDisplayManager::isSwitchable()
{
    if (DMType == NewGDM || DMType == LightDM) {
        QDBusObjectPath currentSeat;
        if (getCurrentSeat(nullptr, &currentSeat)) {
            SystemdSeat SDseat(currentSeat);
            if (SDseat.isValid()) {
                QVariant prop = SDseat.property("CanMultiSession");
                if (prop.isValid())
                    return prop.toBool();
                else {
                    // Newer systemd versions (since 246) don't expose "CanMultiSession" anymore.
                    // It's hidden and always true.
                    // See https://github.com/systemd/systemd/commit/8f8cc84ba4612e74cd1e26898c6816e6e60fc4e9
                    // and https://github.com/systemd/systemd/commit/c2b178d3cacad52eadc30ecc349160bc02d32a9c
                    // So assume that it's supported if the property is invalid.
                    return true;
                }
            }
            CKSeat CKseat(currentSeat);
            if (CKseat.isValid()) {
                QDBusReply<bool> r = CKseat.call(QStringLiteral("CanActivateSessions"));
                if (r.isValid())
                    return r.value();
            }
        }
        return false;
    }

    if (DMType == OldKDM)
        return dpy[0] == ':';

    if (DMType == OldGDM)
        return exec("QUERY_VT\n");

    QByteArray re;

    return exec("caps\n", re) && re.indexOf("\tlocal") >= 0;
}

int KDisplayManager::numReserve()
{
    if (DMType == NewGDM || DMType == OldGDM || DMType == LightDM)
        return 1; /* Bleh */

    if (DMType == OldKDM)
        return strstr(ctl, ",rsvd") ? 1 : -1;

    QByteArray re;
    int p;

    if (!(exec("caps\n", re) && (p = re.indexOf("\treserve ")) >= 0))
        return -1;
    return atoi(re.data() + p + 9);
}

void KDisplayManager::startReserve()
{
    if (DMType == NewGDM)
        GDMFactory().call(QStringLiteral("CreateTransientDisplay"));
    else if (DMType == OldGDM)
        exec("FLEXI_XSERVER\n");
    else if (DMType == LightDM) {
        LightDMDBus lightDM;
        lightDM.call(QStringLiteral("SwitchToGreeter"));
    } else
        exec("reserve\n");
}

bool KDisplayManager::localSessions(SessList &list)
{
    if (DMType == OldKDM)
        return false;

    if (DMType == NewGDM || DMType == LightDM) {
        QDBusObjectPath currentSession, currentSeat;
        if (getCurrentSeat(&currentSession, &currentSeat)) {
            // we'll divide the code in two branches to reduce the overhead of calls to non-existent services
            // systemd part // preferred
            if (QDBusConnection::systemBus().interface()->isServiceRegistered(SYSTEMD_SERVICE)) {
                foreach (const QDBusObjectPath &sp, getSessionsForSeat(currentSeat)) {
                    SystemdSession lsess(sp);
                    if (lsess.isValid()) {
                        SessEnt se;
                        lsess.getSessionLocation(se);
                        if ((lsess.property("Class").toString() != QLatin1String("greeter"))
                            && (lsess.property("State").toString() == QLatin1String("online")
                                || lsess.property("State").toString() == QLatin1String("active"))) {
                            NumberedDBusObjectPath numberedPath = lsess.getUser();
                            se.display = lsess.property("Display").toString();
                            se.vt = lsess.property("VTNr").toInt();
                            se.user = KUser(K_UID(numberedPath.num)).loginName();
                            /* TODO:
                             * regarding the session name in this, it IS possible to find it out - logind tracks the session leader PID
                             * the problem is finding out the name of the process, I could come only with reading /proc/PID/comm which
                             * doesn't seem exactly... right to me --mbriza
                             */
                            se.session = QStringLiteral("<unknown>");

                            se.self = lsess.property("Id").toString() == qgetenv("XDG_SESSION_ID");
                            se.tty = !lsess.property("TTY").toString().isEmpty();
                        }
                        list.append(se);
                    }
                }
            }
            // ConsoleKit part
            else if (QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.ConsoleKit"))) {
                foreach (const QDBusObjectPath &sp, getSessionsForSeat(currentSeat)) {
                    CKSession lsess(sp);
                    if (lsess.isValid()) {
                        SessEnt se;
                        lsess.getSessionLocation(se);
                        // "Warning: we haven't yet defined the allowed values for this property.
                        // It is probably best to avoid this until we do."
                        QDBusReply<QString> r = lsess.call(QStringLiteral("GetSessionType"));
                        if (r.value() != QLatin1String("LoginWindow")) {
                            QDBusReply<unsigned> r2 = lsess.call(QStringLiteral("GetUnixUser"));
                            se.user = KUser(K_UID(r2.value())).loginName();
                            se.session = QStringLiteral("<unknown>");
                        }
                        se.self = (sp == currentSession);
                        list.append(se);
                    }
                }
            } else {
                return false;
            }
            return true;
        }
        return false;
    }

    QByteArray re;

    if (DMType == OldGDM) {
        if (!exec("CONSOLE_SERVERS\n", re))
            return false;
        const QStringList sess = QString(re.data() + 3).split(QChar(';'), Qt::SkipEmptyParts);
        for (QStringList::ConstIterator it = sess.constBegin(); it != sess.constEnd(); ++it) {
            QStringList ts = (*it).split(QChar(','));
            SessEnt se;
            se.display = ts[0];
            se.user = ts[1];
            se.vt = ts[2].toInt();
            se.session = QStringLiteral("<unknown>");
            se.self = ts[0] == ::getenv("DISPLAY"); /* Bleh */
            se.tty = false;
            list.append(se);
        }
    } else {
        if (!exec("list\talllocal\n", re))
            return false;
        const QStringList sess = QString(re.data() + 3).split(QChar('\t'), Qt::SkipEmptyParts);
        for (QStringList::ConstIterator it = sess.constBegin(); it != sess.constEnd(); ++it) {
            QStringList ts = (*it).split(QChar(','));
            SessEnt se;
            se.display = ts[0];
            se.vt = ts[1].midRef(2).toInt();
            se.user = ts[2];
            se.session = ts[3];
            se.self = (ts[4].indexOf('*') >= 0);
            se.tty = (ts[4].indexOf('t') >= 0);
            list.append(se);
        }
    }
    return true;
}

void KDisplayManager::sess2Str2(const SessEnt &se, QString &user, QString &loc)
{
    if (se.tty) {
        user = i18nc("user: ...", "%1: TTY login", se.user);
        loc = se.vt ? QStringLiteral("vt%1").arg(se.vt) : se.display;
    } else {
        user = se.user.isEmpty() ? se.session.isEmpty()
                ? i18nc("... location (TTY or X display)", "Unused")
                : se.session == QLatin1String("<remote>") ? i18n("X login on remote host") : i18nc("... host", "X login on %1", se.session)
                                 : se.session == QLatin1String("<unknown>") ? se.user : i18nc("user: session type", "%1: %2", se.user, se.session);
        loc = se.vt ? QStringLiteral("%1, vt%2").arg(se.display).arg(se.vt) : se.display;
    }
}

QString KDisplayManager::sess2Str(const SessEnt &se)
{
    QString user, loc;

    sess2Str2(se, user, loc);
    return i18nc("session (location)", "%1 (%2)", user, loc);
}

bool KDisplayManager::switchVT(int vt)
{
    if (DMType == NewGDM || DMType == LightDM) {
        QDBusObjectPath currentSeat;
        if (getCurrentSeat(nullptr, &currentSeat)) {
            // systemd part // preferred
            if (QDBusConnection::systemBus().interface()->isServiceRegistered(SYSTEMD_SERVICE)) {
                foreach (const QDBusObjectPath &sp, getSessionsForSeat(currentSeat)) {
                    SystemdSession lsess(sp);
                    if (lsess.isValid()) {
                        SessEnt se;
                        lsess.getSessionLocation(se);
                        if (se.vt == vt) {
                            lsess.call(SYSTEMD_SWITCH_CALL);
                            return true;
                        }
                    }
                }
            }
            // ConsoleKit part
            else if (QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.ConsoleKit"))) {
                foreach (const QDBusObjectPath &sp, getSessionsForSeat(currentSeat)) {
                    CKSession lsess(sp);
                    if (lsess.isValid()) {
                        SessEnt se;
                        lsess.getSessionLocation(se);
                        if (se.vt == vt) {
                            if (se.tty) // ConsoleKit simply ignores these
                                return false;
                            lsess.call(QStringLiteral("Activate"));
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    if (DMType == OldGDM)
        return exec(QStringLiteral("SET_VT %1\n").arg(vt).toLatin1());

    return exec(QStringLiteral("activate\tvt%1\n").arg(vt).toLatin1());
}

void KDisplayManager::lockSwitchVT(int vt)
{
    // Lock first, otherwise the lock won't be able to kick in until the session is re-activated.
    QDBusInterface screensaver(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QStringLiteral("org.freedesktop.ScreenSaver"));
    screensaver.call(QStringLiteral("Lock"));

    switchVT(vt);
}

void KDisplayManager::GDMAuthenticate()
{
    FILE *fp;
    const char *dpy, *dnum, *dne;
    int dnl;
    Xauth *xau;

    dpy = DisplayString(QX11Info::display());
    if (!dpy) {
        dpy = ::getenv("DISPLAY");
        if (!dpy)
            return;
    }
    dnum = strchr(dpy, ':') + 1;
    dne = strchr(dpy, '.');
    dnl = dne ? dne - dnum : strlen(dnum);

    /* XXX should do locking */
    if (!(fp = fopen(XauFileName(), "r")))
        return;

    while ((xau = XauReadAuth(fp))) {
        if (xau->family == FamilyLocal && xau->number_length == dnl && !memcmp(xau->number, dnum, dnl) && xau->data_length == 16 && xau->name_length == 18
            && !memcmp(xau->name, "MIT-MAGIC-COOKIE-1", 18)) {
            QString cmd(QStringLiteral("AUTH_LOCAL "));
            for (int i = 0; i < 16; i++)
                cmd += QString::number((uchar)xau->data[i], 16).rightJustified(2, '0');
            cmd += '\n';
            if (exec(cmd.toLatin1())) {
                XauDisposeAuth(xau);
                break;
            }
        }
        XauDisposeAuth(xau);
    }

    fclose(fp);
}

#endif // HAVE_X11
