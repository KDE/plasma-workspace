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

#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

#include <QDir>
#include <QFile>

#include "kded_interface.h"

#include "ui_freespacenotifier_prefs_base.h"

#include "settings.h"

K_PLUGIN_CLASS_WITH_JSON(FreeSpaceNotifierModule, "freespacenotifier.json")

FreeSpaceNotifierModule::FreeSpaceNotifierModule(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    // If the module is loaded, notifications are enabled
    FreeSpaceNotifierSettings::setEnableNotification(true);

    auto m_notifier = Solid::DeviceNotifier::instance();
    connect(m_notifier, &Solid::DeviceNotifier::deviceAdded, this, [this](const QString &udi) {
        Solid::Device device(udi);

        // Required for two stage devices
        if (auto volume = device.as<Solid::StorageVolume>()) {
            Solid::GenericInterface *iface = device.as<Solid::GenericInterface>();
            if (iface) {
                iface->setProperty("udi", udi);
                connect(iface, &Solid::GenericInterface::propertyChanged, this, [this, udi]() {
                    onNewSolidDevice(udi);
                });
            }
        }
        onNewSolidDevice(udi);
    });
    connect(m_notifier, &Solid::DeviceNotifier::deviceRemoved, this, [this](const QString &udi) {
        stopTracking(udi);
    });

    const auto devices = Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess);
    for (auto device : devices) {
        onNewSolidDevice(device.udi());
    };
}

void FreeSpaceNotifierModule::onNewSolidDevice(const QString &udi)
{
    Solid::Device device(udi);
    Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
    if (!access) {
        return;
    }

    // We only track a partition if we are able to
    // determine that it's not read only.
    bool isReadOnly = true;
    if (auto generic = device.as<Solid::GenericInterface>()) {
        isReadOnly = generic->property(QStringLiteral("ReadOnly")).toBool();
    }
    // Cache devices should be marked through a
    // CACHEDIR.TAG file to avoid indexing; see
    // https://bford.info/cachedir/ for reference.
    const bool isCache = QFile::exists(QDir(access->filePath()).filePath(QStringLiteral("CACHEDIR.TAG")));
    if (isReadOnly || isCache) {
        return;
    }

    if (access->isAccessible()) {
        startTracking(udi, access);
    }
    connect(access, &Solid::StorageAccess::accessibilityChanged, this, [this, udi, access](bool available) {
        if (available) {
            startTracking(udi, access);
        } else {
            stopTracking(udi);
        }
    });
}

void FreeSpaceNotifierModule::startTracking(const QString &udi, Solid::StorageAccess *access)
{
    if (m_notifiers.contains(udi)) {
        return;
    }
    Solid::Device device(udi);

    KLocalizedString message = ki18n("Your %1 partition is running out of disk space; %2 MiB of space remaining (%3%).").subs(device.displayName());
    if (access->filePath() == QStringLiteral("/")) {
        message = ki18n("Your Root partition is running out of disk space; %1 MiB of space remaining (%2%).");
    } else if (access->filePath() == QDir::homePath()) {
        message = ki18n("Your Home folder is running out of disk space; %1 MiB of space remaining (%2%).");
    }
    auto *notifier = new FreeSpaceNotifier(udi, access->filePath(), message, this);
    m_notifiers.insert(udi, notifier);
}

void FreeSpaceNotifierModule::stopTracking(const QString &udi)
{
    if (m_notifiers.contains(udi)) {
        delete m_notifiers.take(udi);
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
