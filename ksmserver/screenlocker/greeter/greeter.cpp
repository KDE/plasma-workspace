/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
Copyright (C) 2002 Luboš Luňák <l.lunak@kde.org>
Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "greeter.h"
#include "kscreensaversettings.h"
// workspace
#include <kcheckpass-enums.h>
// KDE
#include <KDebug>
#include <KLibrary>
#include <KLocale>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KStandardDirs>
#include <KLocalizedString>

// Qt
#include <QtCore/QFile>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTimer>
#include <QGraphicsProxyWidget>
#include <QLineEdit>

// kscreenlocker stuff
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <QPalette>

namespace ScreenLocker
{

Greeter::Greeter(QObject *parent)
    : QObject(parent)
    , m_greeterWidget(new QWidget())
    , m_greet(0)
    , m_valid(false)
    , m_pid(0)
    , m_fd(0)
    , m_notifier(NULL)
    , m_failedLock(false)
{
    m_pluginHandle.library = 0;
    initialize();
    m_valid = loadGreetPlugin();
    if (m_valid) {

        m_greet = m_pluginHandle.info->create(this, m_greeterWidget, QString(),
                                        KGreeterPlugin::Authenticate,
                                        KGreeterPlugin::ExUnlock);
        m_greet->start();
    }
}

Greeter::~Greeter()
{
    if (m_pluginHandle.library) {
        if (m_pluginHandle.info->done) {
            m_pluginHandle.info->done();
        }
        m_pluginHandle.library->unload();
    }
}

void Greeter::initialize()
{
    KScreenSaverSettings::self()->readConfig();
    m_plugins = KScreenSaverSettings::pluginsUnlock();
    if (m_plugins.isEmpty()) {
        m_plugins << QLatin1String( "classic" ) << QLatin1String( "generic" );
    }
    m_pluginOptions = KScreenSaverSettings::pluginOptions();
    const QStringList dmopt =
        QString::fromLatin1( ::getenv( "XDM_MANAGED" )).split(QLatin1Char(','), QString::SkipEmptyParts);
    for (QStringList::ConstIterator it = dmopt.constBegin(); it != dmopt.constEnd(); ++it) {
        if ((*it).startsWith(QLatin1String( "method=" ))) {
            m_method = (*it).mid(7);
        }
    }
}

// standard greeter stuff
// private static
QVariant Greeter::getConf(void *ctx, const char *key, const QVariant &dflt)
{
    Greeter *that = (Greeter *)ctx;
    QString fkey = QLatin1String( key ) % QLatin1Char( '=' );
    for (QStringList::ConstIterator it = that->m_pluginOptions.constBegin();
         it != that->m_pluginOptions.constEnd(); ++it)
        if ((*it).startsWith( fkey ))
            return (*it).mid( fkey.length() );
    return dflt;
}

bool Greeter::loadGreetPlugin()
{
    if (m_pluginHandle.library) {
        //we were locked once before, so all the plugin loading's done already
        //FIXME should I be unloading the plugin on unlock instead?
        return true;
    }
    for (QStringList::ConstIterator it = m_plugins.constBegin(); it != m_plugins.constEnd(); ++it) {
        GreeterPluginHandle plugin;
        KLibrary *lib = new KLibrary( (*it)[0] == QLatin1Char( '/' ) ? *it : QLatin1String( "kgreet_" ) + *it );
        if (lib->fileName().isEmpty()) {
            kWarning() << "GreeterPlugin " << *it << " does not exist" ;
            delete lib;
            continue;
        }
        if (!lib->load()) {
            kWarning() << "Cannot load GreeterPlugin " << *it << " (" << lib->fileName() << ")" ;
            delete lib;
            continue;
        }
        plugin.library = lib;
        plugin.info = (KGreeterPluginInfo *)lib->resolveFunction( "kgreeterplugin_info" );
        if (!plugin.info ) {
            kWarning() << "GreeterPlugin " << *it << " (" << lib->fileName() << ") is no valid greet widget plugin" ;
            lib->unload();
            delete lib;
            continue;
        }
        if (plugin.info->method && !m_method.isEmpty() && m_method != QLatin1String(  plugin.info->method )) {
            kDebug() << "GreeterPlugin " << *it << " (" << lib->fileName() << ") serves " << plugin.info->method << ", not " << m_method;
            lib->unload();
            delete lib;
            continue;
        }
        if (!plugin.info->init( m_method, getConf, this )) {
            kDebug() << "GreeterPlugin " << *it << " (" << lib->fileName() << ") refuses to serve " << m_method;
            lib->unload();
            delete lib;
            continue;
        }
        kDebug() << "GreeterPlugin " << *it << " (" << plugin.info->method << ", " << plugin.info->name << ") loaded";
        m_pluginHandle = plugin;
        return true;
    }
    return false;
}

void Greeter::verify()
{
    if (m_failedLock) {
        // greeter blocked due to failed unlock attempt
        return;
    }
    m_greet->next();
}

void Greeter::clear()
{
    m_greet->clear();
}

void Greeter::failedTimer()
{
    emit greeterReady();
    m_greet->revive();
    m_greet->start();
    m_failedLock = false;
}

////// kckeckpass interface code

int Greeter::Reader(void *buf, int count)
{
    int ret, rlen;

    for (rlen = 0; rlen < count; ) {
      dord:
        ret = ::read(m_fd, (void *)((char *)buf + rlen), count - rlen);
        if (ret < 0) {
            if (errno == EINTR)
                goto dord;
            if (errno == EAGAIN)
                break;
            return -1;
        }
        if (!ret)
            break;
        rlen += ret;
    }
    return rlen;
}

bool Greeter::GRead(void *buf, int count)
{
    return Reader(buf, count) == count;
}

bool Greeter::GWrite(const void *buf, int count)
{
    return ::write(m_fd, buf, count) == count;
}

bool Greeter::GSendInt(int val)
{
    return GWrite(&val, sizeof(val));
}

bool Greeter::GSendStr(const char *buf)
{
    int len = buf ? ::strlen (buf) + 1 : 0;
    return GWrite(&len, sizeof(len)) && GWrite (buf, len);
}

bool Greeter::GSendArr(int len, const char *buf)
{
    return GWrite(&len, sizeof(len)) && GWrite (buf, len);
}

bool Greeter::GRecvInt(int *val)
{
    return GRead(val, sizeof(*val));
}

bool Greeter::GRecvArr(char **ret)
{
    int len;
    char *buf;

    if (!GRecvInt(&len))
        return false;
    if (!len) {
        *ret = 0;
        return true;
    }
    if (!(buf = (char *)::malloc (len)))
        return false;
    *ret = buf;
    if (GRead (buf, len)) {
        return true;
    } else {
        ::free(buf);
        *ret = 0;
        return false;
    }
}

void Greeter::reapVerify()
{
    m_notifier->setEnabled( false );
    m_notifier->deleteLater();
    m_notifier = 0;
    ::close( m_fd );
    int status;
    while (::waitpid( m_pid, &status, 0 ) < 0)
        if (errno != EINTR) { // This should not happen ...
            cantCheck();
            return;
        }
    if (WIFEXITED(status))
        switch (WEXITSTATUS(status)) {
        case AuthOk:
            m_greet->succeeded();
            emit greeterAccepted();
            return;
        case AuthBad:
            m_greet->failed();
            emit greeterFailed();
            m_failedLock = true;
            QTimer::singleShot(1500, this, SLOT(failedTimer()));
            //KNotification::event( QLatin1String( "unlockfailed" ) );*/
            return;
        case AuthAbort:
            return;
        }
    cantCheck();
}

void Greeter::handleVerify()
{
    int ret;
    char *arr;

    if (GRecvInt( &ret )) {
        switch (ret) {
        case ConvGetBinary:
            if (!GRecvArr( &arr ))
                break;
            m_greet->binaryPrompt( arr, false );
            if (arr)
                ::free( arr );
            return;
        case ConvGetNormal:
            if (!GRecvArr( &arr ))
                break;
            m_greet->textPrompt( arr, true, false );
            if (arr)
                ::free( arr );
            return;
        case ConvGetHidden:
            if (!GRecvArr( &arr ))
                break;
            m_greet->textPrompt( arr, false, false );
            if (arr)
                ::free( arr );
            return;
        case ConvPutInfo:
            if (!GRecvArr( &arr ))
                break;
            if (!m_greet->textMessage( arr, false ))
                emit greeterMessage(QString::fromLocal8Bit(arr));
            ::free( arr );
            return;
        case ConvPutError:
            if (!GRecvArr( &arr ))
                break;
            if (!m_greet->textMessage( arr, true ))
                emit greeterMessage(QString::fromLocal8Bit(arr));
            ::free( arr );
            return;
        }
    }
    reapVerify();
}

////// greeter plugin callbacks

void Greeter::gplugReturnText( const char *text, int tag )
{
    GSendStr( text );
    if (text)
        GSendInt( tag );
}

void Greeter::gplugReturnBinary( const char *data )
{
    if (data) {
        unsigned const char *up = (unsigned const char *)data;
        int len = up[3] | (up[2] << 8) | (up[1] << 16) | (up[0] << 24);
        if (!len)
            GSendArr( 4, data );
        else
            GSendArr( len, data );
    } else
        GSendArr( 0, 0 );
}

void Greeter::gplugSetUser( const QString & )
{
    // ignore ...
}

void Greeter::cantCheck()
{
    m_greet->failed();
    emit greeterMessage(i18n("Cannot unlock the session because the authentication system failed to work!"));
    m_greet->revive();
}

//---------------------------------------------------------------------------
//
// Starts the kcheckpass process to check the user's password.
//
void Greeter::gplugStart()
{
    int sfd[2];
    char fdbuf[16];

    if (m_notifier)
        return;
    if (::socketpair(AF_LOCAL, SOCK_STREAM, 0, sfd)) {
        cantCheck();
        return;
    }
    if ((m_pid = ::fork()) < 0) {
        ::close(sfd[0]);
        ::close(sfd[1]);
        cantCheck();
        return;
    }
    if (!m_pid) {
        ::close(sfd[0]);
        sprintf(fdbuf, "%d", sfd[1]);
        execlp(QFile::encodeName(KStandardDirs::findExe(QLatin1String( "kcheckpass" ))).data(),
               "kcheckpass",
               "-m", m_pluginHandle.info->method,
               "-S", fdbuf,
               (char *)0);
        _exit(20);
    }
    ::close(sfd[1]);
    m_fd = sfd[0];
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, SIGNAL(activated(int)), SLOT(handleVerify()));
}

void Greeter::gplugChanged()
{
}

void Greeter::gplugActivity()
{
    // ignore
}

void Greeter::gplugMsgBox(QMessageBox::Icon type, const QString &text)
{
    Q_UNUSED(type)
    emit greeterMessage(text);
}

bool Greeter::gplugHasNode(const QString &)
{
    return false;
}

} // end namespace
#include "greeter.moc"
