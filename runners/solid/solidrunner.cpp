/***************************************************************************
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
#include "solidrunner.h"
#include "devicewrapper.h"

//Qt
#include <QAction>

//KDE
#include <KIcon>

//Plasma
#include <Plasma/Plasma>
#include <Plasma/DataEngineManager>

//Solid
#include <Solid/Device>

using namespace Plasma;

SolidRunner::SolidRunner(QObject* parent, const QVariantList& args)
    : AbstractRunner(parent, args),
      m_deviceList()
{
    Q_UNUSED(args)
    setObjectName( QLatin1String("Solid" ));

    m_engineManager = Plasma::DataEngineManager::self();

    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Finds devices whose name match :q:")));

    setDefaultSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "device"),
                                   i18n("Lists all devices and allows them to be mounted, unmounted or ejected.")));
    addSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "mount"),
                                   i18n("Lists all devices which can be mounted, and allows them to be mounted.")));
    addSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "unlock"),
                                   i18n("Lists all encrypted devices which can be unlocked, and allows them to be unlocked.")));
    addSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "unmount"),
                                   i18n("Lists all devices which can be unmounted, and allows them to be unmounted.")));
    addSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "lock"),
                                   i18n("Lists all encrypted devices which can be locked, and allows them to be locked.")));

    addSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "eject"),
                                   i18n("Lists all devices which can be ejected, and allows them to be ejected.")));

}

SolidRunner::~SolidRunner()
{
}

void SolidRunner::init()
{

  m_hotplugEngine = dataEngine("hotplug");
  m_solidDeviceEngine = dataEngine("soliddevice");

  //connect to engine when a device is plugged
  connect(m_hotplugEngine, SIGNAL(sourceAdded(QString)),
          this, SLOT(onSourceAdded(QString)));
  connect(m_hotplugEngine, SIGNAL(sourceRemoved(QString)),
          this, SLOT(onSourceRemoved(QString)));
  fillPreviousDevices();
}

void SolidRunner::cleanActionsForDevice(DeviceWrapper * dev)
{
    const QStringList actionIds = dev->actionIds();
    if (!actionIds.isEmpty()) {
        foreach (const QString& id, actionIds) {
            removeAction(id);
        }
    }
}

QList<QAction*> SolidRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    QList<QAction*> actions;

    DeviceWrapper* dev = m_deviceList.value(match.data().toString());
    if (dev) {
        QStringList actionIds = dev->actionIds();
        if (!actionIds.isEmpty()) {
            foreach (const QString& id, actionIds) {
                actions << action(id);
            }
        }
    }
    return actions;
}

void SolidRunner::match(Plasma::RunnerContext& context)
{
    m_currentContext = context;
    createOrUpdateMatches(m_deviceList.keys());
}

void SolidRunner::createOrUpdateMatches(const QStringList &udiList)
{
    const QString term = m_currentContext.query();

    if (!m_currentContext.isValid()) {
        return;
    }

    if (!m_currentContext.singleRunnerQueryMode() && (term.length() < 3)) {
        return;
    }
    QList<Plasma::QueryMatch> matches;

    // keyword match: when term starts with "device" we list all devices
    QStringList keywords = term.split(" ");
    QString deviceDescription;
    bool onlyMounted = false;
    bool onlyMountable = false;
    bool onlyEncrypted = false;
    bool onlyOptical = false;
    bool forceEject = false;
    bool showDevices = false;
    if (keywords[0].startsWith(i18nc("Note this is a KRunner keyword", "device") , Qt::CaseInsensitive)) {
        showDevices = true;
        keywords.removeFirst();
    }

    if (!keywords.isEmpty()) {
        if (keywords[0].startsWith(i18nc("Note this is a KRunner keyword", "mount") , Qt::CaseInsensitive)) {
            showDevices = true;
            onlyMountable = true;
            keywords.removeFirst();
        } else if (keywords[0].startsWith(i18nc("Note this is a KRunner keyword", "unmount") , Qt::CaseInsensitive)) {
            showDevices = true;
            onlyMounted = true;
            keywords.removeFirst();
        } else if (keywords[0].startsWith(i18nc("Note this is a KRunner keyword", "eject") , Qt::CaseInsensitive)) {
            showDevices = true;
            onlyOptical = true;
            forceEject = true;
            keywords.removeFirst();
        } else if (keywords[0].startsWith(i18nc("Note this is a KRunner keyword", "unlock") , Qt::CaseInsensitive)) {
            showDevices = true;
            onlyMountable = true;
            onlyEncrypted = true;
            keywords.removeFirst();
        } else if (keywords[0].startsWith(i18nc("Note this is a KRunner keyword", "lock") , Qt::CaseInsensitive)) {
            showDevices = true;
            onlyMounted = true;
            onlyEncrypted = true;
            keywords.removeFirst();
        }
    }

    if (!keywords.isEmpty()) {
        deviceDescription = keywords[0];
    }

    foreach (const QString& udi,  udiList) {
        DeviceWrapper * dev = m_deviceList.value(udi);
        if ((deviceDescription.isEmpty() && showDevices) || dev->description().contains(deviceDescription, Qt::CaseInsensitive)) {
            // This is getting quite messy indeed
            if (((!onlyEncrypted) || (onlyEncrypted && dev->isEncryptedContainer())) &&
                ((!onlyOptical) || (onlyOptical && dev->isOpticalDisc())) &&
                ((onlyMounted && dev->isAccessible()) ||
                 (onlyMountable && dev->isStorageAccess() && !dev->isAccessible()) ||
                 (!onlyMounted && !onlyMountable))) {
                dev->setForceEject(forceEject);
                Plasma::QueryMatch match = deviceMatch(dev);
                if (dev->description().compare(deviceDescription, Qt::CaseInsensitive)) {
                    match.setType(Plasma::QueryMatch::ExactMatch);
                } else if (deviceDescription.isEmpty()) {
                    match.setType(Plasma::QueryMatch::PossibleMatch);
                } else {
                    match.setType(Plasma::QueryMatch::CompletionMatch);
                }
                matches << match;
            }
        }
    }

    if (!matches.isEmpty()) {
        m_currentContext.addMatches(term, matches);
    }
}

Plasma::QueryMatch SolidRunner::deviceMatch(DeviceWrapper * device)
{
    Plasma::QueryMatch match(this);
    match.setId(device->id());
    match.setData(device->id());
    match.setIcon(device->icon());
    match.setText(device->description());

    match.setSubtext(device->defaultAction());

    //Put them in order such that the last added device is on top.
    match.setRelevance(0.5+0.1*qreal(m_udiOrderedList.indexOf(device->id()))/qreal(m_udiOrderedList.count()));

    return match;
}

void SolidRunner::run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& match)
{
    Q_UNUSED(context)

    DeviceWrapper *device = m_deviceList.value(match.data().toString());
    if (device) {
        device->runAction(match.selectedAction());
    }
}

void SolidRunner::registerAction(QString &id, QString icon, QString text, QString desktop)
{
    QAction* action = addAction(id, KIcon(icon), text);
    action->setData(desktop);
}

void SolidRunner::refreshMatch(QString &id)
{
    if (!m_currentContext.isValid()) {
        return;
    }

    QueryMatch match(this);
    match.setId(id);
    m_currentContext.removeMatch(match.id());
    QStringList deviceList;
    deviceList << id;
    createOrUpdateMatches(deviceList);
}

void SolidRunner::onSourceAdded(const QString &name)
{
    DeviceWrapper * device = new DeviceWrapper(name);
    connect(device, SIGNAL(registerAction(QString&,QString,QString,QString)),
            this,  SLOT(registerAction(QString&,QString,QString,QString)));
    connect(device, SIGNAL(refreshMatch(QString&)), this, SLOT(refreshMatch(QString&)));

    m_deviceList.insert(name, device);
    m_udiOrderedList << name;
    m_hotplugEngine->connectSource(name, device);
    m_solidDeviceEngine->connectSource(name, device);
}

void SolidRunner::onSourceRemoved(const QString &name)
{
    DeviceWrapper * device = m_deviceList.value(name);
    if (device) {
            m_hotplugEngine->disconnectSource(name, device);
            m_solidDeviceEngine->disconnectSource(name, device);
            disconnect(device, 0, this, 0);
            cleanActionsForDevice(device);
            m_deviceList.remove(name);
            m_udiOrderedList.removeAll(name);
            if (m_currentContext.isValid()) {
                QueryMatch match(this);
                match.setId(device->id());
                m_currentContext.removeMatch(match.id());
            }
            delete device;
    }
}

void SolidRunner::fillPreviousDevices()
{
    foreach (const QString &source, m_hotplugEngine->sources()) {
        onSourceAdded(source);
    }
}

