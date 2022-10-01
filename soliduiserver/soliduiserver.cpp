/*
    SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
    SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>
    SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>
    SPDX-FileCopyrightText: 2011, 2014 Lukas Tinkl <ltinkl@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "soliduiserver.h"
#include "config-X11.h"
#include "soliduiserver_debug.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QIcon>

#include <KLocalizedString>
#include <KPluginFactory>
#include <KUserTimestamp>

#include <kpassworddialog.h>
#include <kwindowsystem.h>

#include <qt5keychain/keychain.h>

// solid specific includes
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/predicate.h>
#include <solid/storagevolume.h>

K_PLUGIN_CLASS_WITH_JSON(SolidUiServer, "soliduiserver.json")

SolidUiServer::SolidUiServer(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
}

SolidUiServer::~SolidUiServer()
{
}

void SolidUiServer::showPassphraseDialog(const QString &udi, const QString &returnService, const QString &returnObject, uint wId, const QString &appId)
{
    if (m_idToPassphraseDialog.contains(returnService + ':' + udi)) {
        KPasswordDialog *dialog = m_idToPassphraseDialog[returnService + ':' + udi];
        dialog->activateWindow();
        return;
    }

    Solid::Device device(udi);

    KPasswordDialog *dialog = new KPasswordDialog(nullptr, KPasswordDialog::ShowKeepPassword);

    QString label = device.vendor();
    if (!label.isEmpty())
        label += ' ';
    label += device.product();

    dialog->setPrompt(i18n("'%1' needs a password to be accessed. Please enter a password.", label));
    dialog->setIcon(QIcon::fromTheme(device.icon()));
    dialog->setProperty("soliduiserver.udi", udi);
    dialog->setProperty("soliduiserver.returnService", returnService);
    dialog->setProperty("soliduiserver.returnObject", returnObject);

    QString uuid;
    if (device.is<Solid::StorageVolume>())
        uuid = device.as<Solid::StorageVolume>()->uuid();

    // read the password from wallet and prefill it to the dialog
    if (!uuid.isEmpty()) {
        dialog->setProperty("soliduiserver.uuid", uuid);

        auto readJob = new QKeychain::ReadPasswordJob(QString::fromLatin1("SolidLuks"), dialog);
        readJob->setKey(uuid);
        connect(readJob, &QKeychain::ReadPasswordJob::finished, dialog, [dialog, readJob]() {
            dialog->setKeepPassword(true);
            dialog->setPassword(readJob->textData());
        });
    }

    connect(dialog, &KPasswordDialog::gotPassword, this, &SolidUiServer::onPassphraseDialogCompleted);
    connect(dialog, &KPasswordDialog::rejected, this, &SolidUiServer::onPassphraseDialogRejected);

    m_idToPassphraseDialog[returnService + ':' + udi] = dialog;

    reparentDialog(dialog, (WId)wId, appId, true);
    dialog->show();
}

void SolidUiServer::onPassphraseDialogCompleted(const QString &pass, bool keep)
{
    KPasswordDialog *dialog = qobject_cast<KPasswordDialog *>(sender());

    if (dialog) {
        QString returnService = dialog->property("soliduiserver.returnService").toString();
        QString returnObject = dialog->property("soliduiserver.returnObject").toString();
        QDBusInterface returnIface(returnService, returnObject);

        QDBusReply<void> reply = returnIface.call(QStringLiteral("passphraseReply"), pass);

        QString udi = dialog->property("soliduiserver.udi").toString();
        m_idToPassphraseDialog.remove(returnService + ':' + udi);

        if (!reply.isValid()) {
            qCWarning(SOLIDUISERVER_DEBUG) << "Impossible to send the passphrase to the application, D-Bus said: " << reply.error().name() << ", "
                                           << reply.error().message() << Qt::endl;
            return; // don't save into wallet if an error occurs
        }

        if (keep) { // save the password into the wallet
            auto writePassJob = new QKeychain::WritePasswordJob(QString::fromLatin1("SolidLuks"), nullptr);
            const QString uuid = dialog->property("soliduiserver.uuid").toString();
            writePassJob->setKey(uuid);
            writePassJob->setTextData(pass);
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

    dialog->setAttribute(Qt::WA_NativeWindow, true);
    KWindowSystem::setMainWindow(dialog->windowHandle(), wId); // correct, set dialog parent

#if HAVE_X11
    if (modal) {
        KWindowSystem::setState(dialog->winId(), NET::Modal);
    } else {
        KWindowSystem::clearState(dialog->winId(), NET::Modal);
    }
#endif

    // allow dialog activation even if it interrupts, better than trying hacks
    // with keeping the dialog on top or on all desktops
    KUserTimestamp::updateUserTimestamp();
}

#include "soliduiserver.moc"
