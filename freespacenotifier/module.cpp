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

    const QStorageInfo rootInfo(QStringLiteral("/"));
    const QStorageInfo homeInfo(QDir::homePath());

    QMap<QByteArray, QStorageInfo> storageMap;

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
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

    for (const QStorageInfo &storage : storageMap) {
        const char *baseMessage = "Your %1 is running out of disk space, you have %2 MiB remaining (%3%).";
        QString deviceName = (storage == rootInfo) ? QStringLiteral("Root partition")
            : (storage == homeInfo)                ? QStringLiteral("Home folder")
                                                   : QString::fromUtf8(storage.device());

        auto *notifier = new FreeSpaceNotifier(storage.rootPath(), ki18n(baseMessage).subs(deviceName), this);
        connect(notifier, &FreeSpaceNotifier::configureRequested, this, &FreeSpaceNotifierModule::showConfiguration);
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
