/* This file is part of the KDE Project
   Copyright (c) 2006 Lukas Tinkl <ltinkl@suse.cz>
   Copyright (c) 2008 Lubos Lunak <l.lunak@suse.cz>
   Copyright (c) 2009 Ivo Anjo <knuckles@gmail.com>

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
*/

#include "freespacenotifier.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QSpinBox>
#include <QDebug>

#include <QtDBus/QtDBus>

#include <KLocalizedString>
#include <KRun>
#include <KConfigDialog>
#include <KDiskFreeSpaceInfo>

#include "settings.h"
#include "ui_freespacenotifier_prefs_base.h"

FreeSpaceNotifier::FreeSpaceNotifier( QObject* parent )
    : QObject( parent )
    , lastAvailTimer( NULL )
    , notification( NULL )
    , lastAvail( -1 )
{
    // If we are running, notifications are enabled
    FreeSpaceNotifierSettings::setEnableNotification( true );

    connect( &timer, SIGNAL(timeout()), SLOT(checkFreeDiskSpace()) );
    timer.start( 1000 * 60 /* 1 minute */ );
}

FreeSpaceNotifier::~FreeSpaceNotifier()
{
    // The notification is automatically destroyed when it goes away, so we only need to do this if
    // it is still being shown
    if ( notification ) notification->deref();
}

void FreeSpaceNotifier::checkFreeDiskSpace()
{
    if ( notification || !FreeSpaceNotifierSettings::enableNotification() )
        return;
    KDiskFreeSpaceInfo fsInfo = KDiskFreeSpaceInfo::freeSpaceInfo( QDir::homePath() );
    if ( fsInfo.isValid() )
    {
        int limit = FreeSpaceNotifierSettings::minimumSpace(); // MiB
        int availpct = int( 100 * fsInfo.available() / fsInfo.size() );
        qint64 avail = fsInfo.available() / ( 1024 * 1024 ); // to MiB
        bool warn = false;
        if( avail < limit ) // avail disk space dropped under a limit
        {
            if( lastAvail < 0 ) // always warn the first time
            {
                lastAvail = avail;
                warn = true;
            }
            else if( avail > lastAvail ) // the user freed some space
                lastAvail = avail;       // so warn if it goes low again
            else if( avail < lastAvail / 2 ) // available dropped to a half of previous one, warn again
            {
                warn = true;
                lastAvail = avail;
            }
            // do not change lastAvail otherwise, to handle free space slowly going down
        }
        if ( warn )
        {
            notification = new KNotification( QStringLiteral("freespacenotif"), 0, KNotification::Persistent );

            notification->setText( i18nc( "Warns the user that the system is running low on space on his home folder, indicating the percentage and absolute MiB size remaining, and asks if the user wants to do something about it", "You are running low on disk space on your home folder (currently %2%, %1 MiB free).\nWould you like to run a file manager to free some disk space?", avail, availpct ) );
            notification->setActions( QStringList() << i18nc( "Opens a file manager like dolphin", "Open File Manager" ) << i18nc( "Closes the notification", "Do Nothing" ) << i18nc( "Allows the user to configure the warning notification being shown", "Configure Warning" ) );
            //notification->setPixmap( ... ); // TODO: Maybe add a picture here?

            connect( notification, SIGNAL(action1Activated()), SLOT(openFileManager()) );
            connect( notification, SIGNAL(action2Activated()), SLOT(cleanupNotification()) );
            connect( notification, SIGNAL(action3Activated()), SLOT(showConfiguration()) );
            connect( notification, SIGNAL(closed()),           SLOT(cleanupNotification()) );

            notification->setComponentName( QStringLiteral( "freespacenotifier" ) );
            notification->sendEvent();
        }
    }
}

void FreeSpaceNotifier::openFileManager()
{
    cleanupNotification();
    new KRun( QUrl::fromLocalFile( QDir::homePath() ), 0 );
}

void FreeSpaceNotifier::showConfiguration()
{
    cleanupNotification();

    if ( KConfigDialog::showDialog( QStringLiteral("settings") ) )  {
        return;
    }

    KConfigDialog *dialog = new KConfigDialog( 0, QStringLiteral("settings"), FreeSpaceNotifierSettings::self() );
    QWidget *generalSettingsDlg = new QWidget();

    Ui::freespacenotifier_prefs_base preferences;
    preferences.setupUi( generalSettingsDlg );

    dialog->addPage( generalSettingsDlg, i18nc( "The settings dialog main page name, as in 'general settings'", "General" ),
                     QStringLiteral("system-run") );
    connect( dialog, SIGNAL(finished()), this, SLOT(configDialogClosed()) );
    dialog->setAttribute( Qt::WA_DeleteOnClose );
    dialog->show();
}

void FreeSpaceNotifier::cleanupNotification()
{
    notification = NULL;

    // warn again if constantly below limit for too long
    if( lastAvailTimer == NULL )
    {
        lastAvailTimer = new QTimer( this );
        connect( lastAvailTimer, SIGNAL(timeout()), SLOT(resetLastAvailable()) );
    }
    lastAvailTimer->start( 1000 * 60 * 60 /* 1 hour*/ );
}

void FreeSpaceNotifier::resetLastAvailable()
{
    lastAvail = -1;
    lastAvailTimer->deleteLater();
    lastAvailTimer = NULL;
}

void FreeSpaceNotifier::configDialogClosed()
{
    if ( !FreeSpaceNotifierSettings::enableNotification() )
        disableFSNotifier();
}

/* The idea here is to disable ourselves by telling kded to stop autostarting us, and
 * to kill the current running instance.
 */
void FreeSpaceNotifier::disableFSNotifier()
{
    QDBusInterface iface( QStringLiteral("org.kde.kded5"),
                          QStringLiteral("/kded"),
                          QStringLiteral("org.kde.kded5") );
    if ( dbusError( iface ) ) return;

    // Disable current module autoload
    iface.call( QStringLiteral("setModuleAutoloading"), QStringLiteral("freespacenotifier"), false );
    if ( dbusError( iface ) ) return;

    // Unload current module
    iface.call( QStringLiteral("unloadModule"), QStringLiteral("freespacenotifier") );
    if ( dbusError( iface ) ) return;
}

bool FreeSpaceNotifier::dbusError( QDBusInterface &iface )
{
    QDBusError err = iface.lastError();
    if ( err.isValid() )
    {
        qCritical() << "Failed to perform operation on kded [" << err.name() << "]:" << err.message();
        return true;
    }
    return false;
}
