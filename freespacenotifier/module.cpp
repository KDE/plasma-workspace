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

    const QString rootPath = QStringLiteral("/");
    const QString homePath = QDir::homePath();

    const auto homeMountPoint = KMountPoint::currentMountPoints().findByPath(homePath);

    if (!homeMountPoint || !homeMountPoint->mountOptions().contains(QLatin1String("ro"))) {
        auto *homeNotifier = new FreeSpaceNotifier(homePath, ki18n("Your Home folder is running out of disk space, you have %1 MiB remaining (%2%)."), this);
        connect(homeNotifier, &FreeSpaceNotifier::configureRequested, this, &FreeSpaceNotifierModule::showConfiguration);
    }

    // If Home is on a separate partition from Root, warn for it, too.
    if (KMountPoint::Ptr rootMountPoint; !homeMountPoint
        || (homeMountPoint->mountPoint() != rootPath
            && (!(rootMountPoint = KMountPoint::currentMountPoints().findByPath(rootPath)) || !rootMountPoint->mountOptions().contains(QLatin1String("ro"))))) {
        auto *rootNotifier = new FreeSpaceNotifier(rootPath, ki18n("Your Root partition is running out of disk space, you have %1 MiB remaining (%2%)."), this);
        connect(rootNotifier, &FreeSpaceNotifier::configureRequested, this, &FreeSpaceNotifierModule::showConfiguration);
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
            org::kde::kded5 kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/kded"), QDBusConnection::sessionBus());
            kded.setModuleAutoloading(QStringLiteral("freespacenotifier"), false);
            kded.unloadModule(QStringLiteral("freespacenotifier"));
        }
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

#include "module.moc"
