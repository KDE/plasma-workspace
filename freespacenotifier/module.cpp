/*
    SPDX-FileCopyrightText: 2006 Lukas Tinkl <ltinkl@suse.cz>
    SPDX-FileCopyrightText: 2008 Lubos Lunak <l.lunak@suse.cz>
    SPDX-FileCopyrightText: 2009 Ivo Anjo <knuckles@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "module.h"

#include <KConfigDialog>
#include <KMountPoint>
#include <KPluginFactory>

#include <QDir>

#include "kded_interface.h"

#include "ui_freespacenotifier_prefs_base.h"

#include "settings.h"

K_PLUGIN_CLASS_WITH_JSON(FreeSpaceNotifierModule, "freespacenotifier.json")

FreeSpaceNotifierModule::FreeSpaceNotifierModule(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    // If the module is loaded, notifications are enabled
    FreeSpaceNotifierSettings::setEnableNotification(true);

    updateNotifiers();

    connect(&m_timer, &QTimer::timeout, this, &FreeSpaceNotifierModule::updateNotifiers);
    m_timer.start(std::chrono::minutes(1));
}

void FreeSpaceNotifierModule::updateNotifiers()
{
    const QStorageInfo rootInfo(QStringLiteral("/"));
    const QStorageInfo homeInfo(QDir::homePath());

    QMap<QByteArray, QStorageInfo> storageMap;

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        qDebug() << "INFO:" << storage;
        if (storage.isValid() && storage.isReady() && !storage.isReadOnly()) {
            QByteArray device = storage.device();
            if (!storageMap.contains(device)) {
                storageMap.insert(device, storage);
            } else if (storage == rootInfo) {
                storageMap[device] = storage;
            } else if (storage == homeInfo && storageMap[device] != rootInfo) {
                storageMap[device] = storage;
            }
        }
    }

    QSet<QByteArray> storageKeys(storageMap.keyBegin(), storageMap.keyEnd());
    QSet<QByteArray> notifierKeys(m_notifiers.keyBegin(), m_notifiers.keyEnd());

    // Devices that have been added since last check
    for (const QByteArray &device : storageKeys - notifierKeys) {
        const QStorageInfo &storage = storageMap[device];
        KLocalizedString message = ki18n("Your %1 partition is running out of disk space, you have %2 MiB remaining (%3%).").subs(QString::fromUtf8(device));
        if (storage == rootInfo) {
            message = ki18n("Your Root partition is running out of disk space, you have %1 MiB remaining (%2%).");
        } else if (storage == homeInfo) {
            message = ki18n("Your Home folder is running out of disk space, you have %1 MiB remaining (%2%).");
        }
        auto *notifier = new FreeSpaceNotifier(storage.rootPath(), message, this);
        connect(notifier, &FreeSpaceNotifier::configureRequested, this, &FreeSpaceNotifierModule::showConfiguration);
        m_notifiers[device] = notifier;
    }

    // Devices that have been removed since last check
    QSet<QByteArray> onlyInNotifiers = notifierKeys - storageKeys;
    for (const QByteArray &device : onlyInNotifiers) {
        delete m_notifiers.take(device);
    }
}

void FreeSpaceNotifierModule::showConfiguration()
{
    if (KConfigDialog::showDialog(QStringLiteral("settings"))) {
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(nullptr, QStringLiteral("settings"), FreeSpaceNotifierSettings::self());
    QWidget *generalSettingsDlg = new QWidget();

    Ui::freespacenotifier_prefs_base preferences;
    preferences.setupUi(generalSettingsDlg);

    dialog->addPage(generalSettingsDlg, i18nc("The settings dialog main page name, as in 'general settings'", "General"), QStringLiteral("system-run"));

    connect(dialog, &KConfigDialog::finished, this, [] {
        if (!FreeSpaceNotifierSettings::enableNotification()) {
            // The idea here is to disable ourselves by telling kded to stop autostarting us, and
            // to kill the current running instance.
            org::kde::kded6 kded(QStringLiteral("org.kde.kded6"), QStringLiteral("/kded"), QDBusConnection::sessionBus());
            kded.setModuleAutoloading(QStringLiteral("freespacenotifier"), false);
            kded.unloadModule(QStringLiteral("freespacenotifier"));
        }
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

#include "module.moc"

#include "moc_module.cpp"
