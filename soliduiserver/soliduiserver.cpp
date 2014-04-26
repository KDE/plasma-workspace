/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>
   Copyright (c) 2007 Alexis Ménard <darktears31@gmail.com>
   Copyright (c) 2011 Lukas Tinkl <ltinkl@redhat.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "soliduiserver.h"

#include <QFile>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusError>
#include <kjobuidelegate.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

#include <kdesktopfileactions.h>
#include <kwindowsystem.h>
#include <kpassworddialog.h>
#include <kwallet.h>
#include <kicon.h>
#include <solid/storagevolume.h>

#include "deviceactionsdialog.h"
#include "deviceaction.h"
#include "deviceserviceaction.h"
#include "devicenothingaction.h"


#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <QStandardPaths>

K_PLUGIN_FACTORY(SolidUiServerFactory,
                 registerPlugin<SolidUiServer>();
    )
K_EXPORT_PLUGIN(SolidUiServerFactory("soliduiserver"))


SolidUiServer::SolidUiServer(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent)
{
}

SolidUiServer::~SolidUiServer()
{
}

void SolidUiServer::showActionsDialog(const QString &udi,
                                      const QStringList &desktopFiles)
{
    if (m_udiToActionsDialog.contains(udi)) {
        DeviceActionsDialog *dialog = m_udiToActionsDialog[udi];
        dialog->activateWindow();
        return;
    }


    QList<DeviceAction*> actions;

    foreach (const QString &desktop, desktopFiles) {
        QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "solid/actions/"+desktop);

        QList<KServiceAction> services
            = KDesktopFileActions::userDefinedServices(filePath, true);

        foreach (const KServiceAction &service, services) {
            DeviceServiceAction *action = new DeviceServiceAction();
            action->setService(service);
            actions << action;
        }
    }

    // Only one action, execute directly
    if (actions.size()==1) {
        DeviceAction *action = actions.takeFirst();
        Solid::Device device(udi);
        action->execute(device);
        delete action;
        return;
    }

    actions << new DeviceNothingAction();

    DeviceActionsDialog *dialog = new DeviceActionsDialog();
    dialog->setDevice(Solid::Device(udi));
    dialog->setActions(actions);

    connect(dialog, SIGNAL(finished()),
            this, SLOT(onActionDialogFinished()));

    m_udiToActionsDialog[udi] = dialog;

    // Update user activity timestamp, otherwise the notification dialog will be shown
    // in the background due to focus stealing prevention. Entering a new media can
    // be seen as a kind of user activity after all. It'd be better to update the timestamp
    // as soon as the media is entered, but it apparently takes some time to get here.
    kapp->updateUserTimestamp();

    dialog->show();
}

void SolidUiServer::onActionDialogFinished()
{
    DeviceActionsDialog *dialog = qobject_cast<DeviceActionsDialog*>(sender());

    if (dialog) {
        QString udi = dialog->device().udi();
        m_udiToActionsDialog.remove(udi);
    }
}


void SolidUiServer::showPassphraseDialog(const QString &udi,
                                         const QString &returnService, const QString &returnObject,
                                         uint wId, const QString &appId)
{
    if (m_idToPassphraseDialog.contains(returnService+':'+udi)) {
        KPasswordDialog *dialog = m_idToPassphraseDialog[returnService+':'+udi];
        dialog->activateWindow();
        return;
    }

    Solid::Device device(udi);

    KPasswordDialog *dialog = new KPasswordDialog(0, KPasswordDialog::ShowKeepPassword);

    QString label = device.vendor();
    if (!label.isEmpty()) label+=' ';
    label+= device.product();

    dialog->setPrompt(i18n("'%1' needs a password to be accessed. Please enter a password.", label));
    dialog->setPixmap(KIcon(device.icon()).pixmap(64, 64));
    dialog->setProperty("soliduiserver.udi", udi);
    dialog->setProperty("soliduiserver.returnService", returnService);
    dialog->setProperty("soliduiserver.returnObject", returnObject);

    QString uuid;
    if (device.is<Solid::StorageVolume>())
        uuid = device.as<Solid::StorageVolume>()->uuid();

    // read the password from wallet and prefill it to the dialog
    if (!uuid.isEmpty()) {
        dialog->setProperty("soliduiserver.uuid", uuid);

        KWallet::Wallet * wallet = KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), (WId) wId);
        const QString folderName = QString::fromLatin1("SolidLuks");
        if (wallet && wallet->hasFolder(folderName)) {
            wallet->setFolder(folderName);
            QString savedPassword;
            if (wallet->readPassword(uuid, savedPassword) == 0) {
                dialog->setKeepPassword(true);
                dialog->setPassword(savedPassword);
            }
            wallet->closeWallet(wallet->walletName(), false);
        }
        delete wallet;
    }


    connect(dialog, SIGNAL(gotPassword(const QString&, bool)),
            this, SLOT(onPassphraseDialogCompleted(const QString&, bool)));
    connect(dialog, SIGNAL(rejected()),
            this, SLOT(onPassphraseDialogRejected()));

    m_idToPassphraseDialog[returnService+':'+udi] = dialog;

    reparentDialog(dialog, (WId)wId, appId, true);
    dialog->show();
}

void SolidUiServer::onPassphraseDialogCompleted(const QString &pass, bool keep)
{
    KPasswordDialog *dialog = qobject_cast<KPasswordDialog*>(sender());

    if (dialog) {
        QString returnService = dialog->property("soliduiserver.returnService").toString();
        QString returnObject = dialog->property("soliduiserver.returnObject").toString();
        QDBusInterface returnIface(returnService, returnObject);

        QDBusReply<void> reply = returnIface.call("passphraseReply", pass);

        QString udi = dialog->property("soliduiserver.udi").toString();
        m_idToPassphraseDialog.remove(returnService+':'+udi);

        if (!reply.isValid()) {
            kWarning() << "Impossible to send the passphrase to the application, D-Bus said: "
                       << reply.error().name() << ", " << reply.error().message() << endl;
            return;  // don't save into wallet if an error occurs
        }

        if (keep)  { // save the password into the wallet
            KWallet::Wallet * wallet = KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), 0);
            if (wallet) {
                const QString folderName = QString::fromLatin1("SolidLuks");
                const QString uuid = dialog->property("soliduiserver.uuid").toString();
                if (!wallet->hasFolder(folderName))
                    wallet->createFolder(folderName);
                if (wallet->setFolder(folderName))
                    wallet->writePassword(uuid, pass);
                wallet->closeWallet(wallet->walletName(), false);
                delete wallet;
            }
        }
    }
}

void SolidUiServer::onPassphraseDialogRejected()
{
    onPassphraseDialogCompleted(QString(), false);
}

void SolidUiServer::reparentDialog(QWidget *dialog, WId wId, const QString &appId, bool modal)
{
    Q_UNUSED(appId);
    // Code borrowed from kwalletd

    KWindowSystem::setMainWindow(dialog, wId); // correct, set dialog parent

#ifdef Q_WS_X11
    if (modal) {
        KWindowSystem::setState(dialog->winId(), NET::Modal);
    } else {
        KWindowSystem::clearState(dialog->winId(), NET::Modal);
    }
#endif

    // allow dialog activation even if it interrupts, better than trying hacks
    // with keeping the dialog on top or on all desktops
    kapp->updateUserTimestamp();
}

#include "soliduiserver.moc"
