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
#include <QMenu>
#include <QDebug>

#include <QtDBus/QtDBus>

#include <KLocalizedString>
#include <KRun>
#include <KConfigDialog>
#include <KDiskFreeSpaceInfo>
#include <KStatusNotifierItem>
#include <KNotification>

#include "settings.h"
#include "ui_freespacenotifier_prefs_base.h"

FreeSpaceNotifier::FreeSpaceNotifier( QObject* parent )
    : QObject( parent )
    , lastAvailTimer( NULL )
    , notification( NULL )
    , lastAvail( -1 )
    , sni(NULL)
{
    // If we are running, notifications are enabled
    FreeSpaceNotifierSettings::setEnableNotification( true );

    connect( &timer, SIGNAL(timeout()), SLOT(checkFreeDiskSpace()) );
    timer.start( 1000 * 60 /* 1 minute */ );

    QTimer::singleShot(0, this, SLOT(checkFreeDiskSpace()));
}

FreeSpaceNotifier::~FreeSpaceNotifier()
{
    // The notification is automatically destroyed when it goes away, so we only need to do this if
    // it is still being shown
    if (notification) {
        notification->close();
    }

    if (sni) {
        sni->deleteLater();
    }
}

void FreeSpaceNotifier::checkFreeDiskSpace()
{
    if (!FreeSpaceNotifierSettings::enableNotification()) {
        // do nothing if notifying is disabled;
        // also stop the timer that probably got us here in the first place
        timer.stop();

        return;
    }

    KDiskFreeSpaceInfo fsInfo = KDiskFreeSpaceInfo::freeSpaceInfo(QDir::homePath());
    if (fsInfo.isValid()) {
        int limit = FreeSpaceNotifierSettings::minimumSpace(); // MiB
        qint64 avail = fsInfo.available() / ( 1024 * 1024 ); // to MiB
        bool warn = false;

        if (avail < limit) {
            // avail disk space dropped under a limit
            if (lastAvail < 0 || avail < lastAvail / 2) { // always warn the first time or when available dropped to a half of previous one, warn again
                lastAvail = avail;
                warn = true;
            } else if (avail > lastAvail) {     // the user freed some space
                lastAvail = avail;              // so warn if it goes low again
            }
            // do not change lastAvail otherwise, to handle free space slowly going down

            if (warn) {
                int availpct = int(100 * fsInfo.available() / fsInfo.size());
                if (!sni) {
                    sni = new KStatusNotifierItem(QStringLiteral("freespacenotifier"));
                    sni->setIconByName(QStringLiteral("drive-harddisk"));
                    sni->setOverlayIconByName(QStringLiteral("dialog-warning"));
                    sni->setTitle(i18n("Low Disk Space"));
                    sni->setCategory(KStatusNotifierItem::Hardware);

                    QMenu *sniMenu = new QMenu();
                    QAction *action = new QAction(i18nc("Opens a file manager like dolphin", "Open File Manager..."), 0);
                    connect(action, &QAction::triggered, this, &FreeSpaceNotifier::openFileManager);
                    sniMenu->addAction(action);

                    action = new QAction(i18nc("Allows the user to configure the warning notification being shown", "Configure Warning..."), 0);
                    connect(action, &QAction::triggered, this, &FreeSpaceNotifier::showConfiguration);
                    sniMenu->addAction(action);

                    action = new QAction(i18nc("Allows the user to hide this notifier item", "Hide"), 0);
                    connect(action, &QAction::triggered, this, &FreeSpaceNotifier::hideSni);
                    sniMenu->addAction(action);

                    sni->setContextMenu(sniMenu);
                    sni->setStandardActionsEnabled(false);
                }

                sni->setStatus(KStatusNotifierItem::NeedsAttention);
                sni->setToolTip(QStringLiteral("drive-harddisk"), i18n("Low Disk Space"), i18n("Remaining space in your Home folder: %1 MiB", QLocale::system().toString(avail)));

                notification = new KNotification(QStringLiteral("freespacenotif"));

                notification->setText(i18nc("Warns the user that the system is running low on space on his home folder, indicating the percentage and absolute MiB size remaining",
                                            "Your Home folder is running out of disk space, you have %1 MiB remaining (%2%)", QLocale::system().toString(avail), availpct));

                connect( notification, SIGNAL(closed()),           SLOT(cleanupNotification()) );

                notification->setComponentName( QStringLiteral( "freespacenotifier" ) );
                notification->sendEvent();
            }
        }
    } else {
        // free space is above limit again, remove the SNI
        if (sni) {
            sni->deleteLater();
            sni = NULL;
        }
    }
}

void FreeSpaceNotifier::hideSni()
{
    if (sni) {
        sni->setStatus(KStatusNotifierItem::Passive);
        QAction *action = qobject_cast<QAction*>(sender());
        if (action) {
            action->setDisabled(true);
        }
    }
}

void FreeSpaceNotifier::openFileManager()
{
    cleanupNotification();
    new KRun( QUrl::fromLocalFile( QDir::homePath() ), 0 );

    if (sni) {
        sni->setStatus(KStatusNotifierItem::Active);
    }
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

    if (sni) {
        sni->setStatus(KStatusNotifierItem::Active);
    }
}

void FreeSpaceNotifier::cleanupNotification()
{
    if (notification) {
        notification->close();
    }
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
    if (!FreeSpaceNotifierSettings::enableNotification()) {
        disableFSNotifier();
    }
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
