/**************************************************************************
 *   Copyright 2009 by Jacopo De Simoi <wilderkde@gmail.com>               *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

//own
#include "devicewrapper.h"

//Qt
#include <QAction>
#include <QTimer>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

//Solid
#include <Solid/Device>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/OpticalDrive>
#include <Solid/OpticalDisc>

//KDE
#include <KIcon>
#include <KMessageBox>
#include <KStandardDirs>
#include <kdesktopfileactions.h>

//Plasma
#include <Plasma/DataEngine>

DeviceWrapper::DeviceWrapper(const QString& udi)
  : m_device(udi),
    m_isStorageAccess(false),
    m_isAccessible(false),
    m_isEncryptedContainer(false)
{
    m_udi = m_device.udi();
}

DeviceWrapper::~DeviceWrapper()
{
}

void DeviceWrapper::dataUpdated(const QString &source, Plasma::DataEngine::Data data)
{
    Q_UNUSED(source)

    if (data.isEmpty()) {
        return;
    }
    if (data["text"].isValid()) {
        m_actionIds.clear();
        foreach (const QString &desktop, data["predicateFiles"].toStringList()) {
            QString filePath = KStandardDirs::locate("data", "solid/actions/" + desktop);
            QList<KServiceAction> services = KDesktopFileActions::userDefinedServices(filePath, true);

            foreach (KServiceAction serviceAction, services) {
                QString actionId = id()+'_'+desktop+'_'+serviceAction.name();
                m_actionIds << actionId;
                emit registerAction(actionId,  serviceAction.icon(), serviceAction.text(), desktop);
            }
        }
        m_isEncryptedContainer = data["isEncryptedContainer"].toBool();
    } else {
        if (data["Device Types"].toStringList().contains("Storage Access")) {
            m_isStorageAccess = true;
            if (data["Accessible"].toBool() == true) {
                m_isAccessible = true;
            } else {
                m_isAccessible = false;
            }
        } else {
            m_isStorageAccess = false;
        }
        if (data["Device Types"].toStringList().contains("OpticalDisc")) {
            m_isOpticalDisc = true;
        } else {
            m_isOpticalDisc = false;
        }
    }

    m_emblems = m_device.emblems();
    m_description = m_device.description();
    m_iconName = m_device.icon();

    emit refreshMatch(m_udi);
}

QString DeviceWrapper::id() const {
     return m_udi;
}

Solid::Device DeviceWrapper::device() const {
    return m_device;
}

KIcon DeviceWrapper::icon() const {
    return KIcon(m_iconName, NULL, m_emblems);
}

bool DeviceWrapper::isStorageAccess() const {
    return m_isStorageAccess;
}

bool DeviceWrapper::isAccessible() const {
    return m_isAccessible;
}

bool DeviceWrapper::isEncryptedContainer() const {
    return m_isEncryptedContainer;
}

bool DeviceWrapper::isOpticalDisc() const {
    return m_isOpticalDisc;
}

QString DeviceWrapper::description() const {
    return m_description;
}

void DeviceWrapper::setForceEject(bool force)
{
    m_forceEject = force;
}

QString DeviceWrapper::defaultAction() const {

    QString actionString;

    if (m_isOpticalDisc && m_forceEject) {
        actionString = i18n("Eject medium");
    } else if (m_isStorageAccess) {
        if (!m_isEncryptedContainer) {
            if (!m_isAccessible) {
                actionString = i18n("Mount the device");
            } else {
                actionString = i18n("Unmount the device");
            }
        } else {
            if (!m_isAccessible) {
                actionString = i18nc("Unlock the encrypted container; will ask for a password; partitions inside will appear as they had been plugged in","Unlock the container");
            } else {
                actionString = i18nc("Close the encrypted container; partitions inside will disappear as they had been unplugged", "Lock the container");
            }
        }
    } else {
            actionString = i18n("Eject medium");
    }
    return actionString;
}

void DeviceWrapper::runAction(QAction * action)
{
    if (action) {
        QString desktopAction = action->data().toString();
        if (!desktopAction.isEmpty()) {
            QStringList desktopFiles;
            desktopFiles.append(desktopAction);
            QDBusInterface soliduiserver("org.kde.kded5", "/modules/soliduiserver", "org.kde.SolidUiServer");
            soliduiserver.asyncCall("showActionsDialog", id(), desktopFiles);
        }
    } else {
        if (isOpticalDisc() && m_forceEject) {
            Solid::OpticalDrive *drive = m_device.parent().as<Solid::OpticalDrive>();
            if (drive) {
                drive->eject();
            }
            return;
        }

        if (m_device.is<Solid::StorageVolume>()) {
            Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
            if (access) {
                if (access->isAccessible()) {
                    access->teardown();
                } else {
                    access->setup();
                }
                return;
            }
        }

        if (isOpticalDisc()) {
            Solid::OpticalDrive *drive = m_device.parent().as<Solid::OpticalDrive>();
            if (drive) {
                drive->eject();
            }
        }
    }
}

QStringList DeviceWrapper::actionIds() const
{
    return m_actionIds;
}

