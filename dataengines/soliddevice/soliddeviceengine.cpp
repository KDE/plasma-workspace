/*
 *   Copyright (C) 2007 Christopher Blauvelt <cblauvelt@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "soliddeviceengine.h"
#include "soliddeviceservice.h"

#include <QDateTime>
#include <QMetaEnum>
#include <Solid/GenericInterface>
#include <klocalizedstring.h>

#include <KFormat>
#include <KNotification>
#include <QApplication>
#include <QDebug>

#include <Plasma/DataContainer>

// TODO: implement in libsolid2
namespace
{
template<class DevIface> DevIface *getAncestorAs(const Solid::Device &device)
{
    for (Solid::Device parent = device.parent(); parent.isValid(); parent = parent.parent()) {
        if (parent.is<DevIface>()) {
            return parent.as<DevIface>();
        }
    }
    return nullptr;
}
}

SolidDeviceEngine::SolidDeviceEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
    , m_temperature(nullptr)
    , m_notifier(nullptr)
{
    Q_UNUSED(args)
    m_signalmanager = new DeviceSignalMapManager(this);

    listenForNewDevices();
    setMinimumPollingInterval(1000);
    connect(this, &Plasma::DataEngine::sourceRemoved, this, &SolidDeviceEngine::sourceWasRemoved);
}

SolidDeviceEngine::~SolidDeviceEngine()
{
}

Plasma::Service *SolidDeviceEngine::serviceForSource(const QString &source)
{
    return new SolidDeviceService(this, source);
}

void SolidDeviceEngine::listenForNewDevices()
{
    if (m_notifier) {
        return;
    }

    // detect when new devices are added
    m_notifier = Solid::DeviceNotifier::instance();
    connect(m_notifier, &Solid::DeviceNotifier::deviceAdded, this, &SolidDeviceEngine::deviceAdded);
    connect(m_notifier, &Solid::DeviceNotifier::deviceRemoved, this, &SolidDeviceEngine::deviceRemoved);
}

bool SolidDeviceEngine::sourceRequestEvent(const QString &name)
{
    if (name.startsWith('/')) {
        Solid::Device device = Solid::Device(name);
        if (device.isValid()) {
            if (m_devicemap.contains(name)) {
                return true;
            } else {
                m_devicemap[name] = device;
                return populateDeviceData(name);
            }
        }
    } else {
        Solid::Predicate predicate = Solid::Predicate::fromString(name);
        if (predicate.isValid() && !m_predicatemap.contains(name)) {
            foreach (const Solid::Device &device, Solid::Device::listFromQuery(predicate)) {
                m_predicatemap[name] << device.udi();
            }

            setData(name, m_predicatemap[name]);
            return true;
        }
    }

    qDebug() << "Source is not a predicate or a device.";
    return false;
}

void SolidDeviceEngine::sourceWasRemoved(const QString &source)
{
    m_devicemap.remove(source);
    m_predicatemap.remove(source);
}

bool SolidDeviceEngine::populateDeviceData(const QString &name)
{
    Solid::Device device = m_devicemap.value(name);
    if (!device.isValid()) {
        return false;
    }

    QStringList devicetypes;
    setData(name, I18N_NOOP("Parent UDI"), device.parentUdi());
    setData(name, I18N_NOOP("Vendor"), device.vendor());
    setData(name, I18N_NOOP("Product"), device.product());
    setData(name, I18N_NOOP("Description"), device.description());
    setData(name, I18N_NOOP("Icon"), device.icon());
    setData(name, I18N_NOOP("Emblems"), device.emblems());
    setData(name, I18N_NOOP("State"), Idle);
    setData(name, I18N_NOOP("Operation result"), Working);
    setData(name, I18N_NOOP("Timestamp"), QDateTime::currentDateTimeUtc());

    if (device.is<Solid::Processor>()) {
        Solid::Processor *processor = device.as<Solid::Processor>();
        if (!processor) {
            return false;
        }

        devicetypes << I18N_NOOP("Processor");
        setData(name, I18N_NOOP("Number"), processor->number());
        setData(name, I18N_NOOP("Max Speed"), processor->maxSpeed());
        setData(name, I18N_NOOP("Can Change Frequency"), processor->canChangeFrequency());
    }
    if (device.is<Solid::Block>()) {
        Solid::Block *block = device.as<Solid::Block>();
        if (!block) {
            return false;
        }

        devicetypes << I18N_NOOP("Block");
        setData(name, I18N_NOOP("Major"), block->deviceMajor());
        setData(name, I18N_NOOP("Minor"), block->deviceMinor());
        setData(name, I18N_NOOP("Device"), block->device());
    }
    if (device.is<Solid::StorageAccess>()) {
        Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
        if (!storageaccess) {
            return false;
        }

        devicetypes << I18N_NOOP("Storage Access");
        setData(name, I18N_NOOP("Accessible"), storageaccess->isAccessible());
        setData(name, I18N_NOOP("File Path"), storageaccess->filePath());

        if (storageaccess->isAccessible()) {
            updateStorageSpace(name);
        }

        m_signalmanager->mapDevice(storageaccess, device.udi());
    }

    if (device.is<Solid::StorageDrive>()) {
        Solid::StorageDrive *storagedrive = device.as<Solid::StorageDrive>();
        if (!storagedrive) {
            return false;
        }

        devicetypes << I18N_NOOP("Storage Drive");

        QStringList bus;
        bus << I18N_NOOP("Ide") << I18N_NOOP("Usb") << I18N_NOOP("Ieee1394") << I18N_NOOP("Scsi") << I18N_NOOP("Sata") << I18N_NOOP("Platform");
        QStringList drivetype;
        drivetype << I18N_NOOP("Hard Disk") << I18N_NOOP("Cdrom Drive") << I18N_NOOP("Floppy") << I18N_NOOP("Tape") << I18N_NOOP("Compact Flash")
                  << I18N_NOOP("Memory Stick") << I18N_NOOP("Smart Media") << I18N_NOOP("SdMmc") << I18N_NOOP("Xd");

        setData(name, I18N_NOOP("Bus"), bus.at((int)storagedrive->bus()));
        setData(name, I18N_NOOP("Drive Type"), drivetype.at((int)storagedrive->driveType()));
        setData(name, I18N_NOOP("Removable"), storagedrive->isRemovable());
        setData(name, I18N_NOOP("Hotpluggable"), storagedrive->isHotpluggable());

        updateHardDiskTemperature(name);
    } else {
        bool isRemovable = false;
        bool isHotpluggable = false;
        Solid::StorageDrive *drive = getAncestorAs<Solid::StorageDrive>(device);
        if (drive) {
            // remove check for isHotpluggable() when plasmoids are changed to check for both properties
            isRemovable = (drive->isRemovable() || drive->isHotpluggable());
            isHotpluggable = drive->isHotpluggable();
        }
        setData(name, I18N_NOOP("Removable"), isRemovable);
        setData(name, I18N_NOOP("Hotpluggable"), isHotpluggable);
    }

    if (device.is<Solid::OpticalDrive>()) {
        Solid::OpticalDrive *opticaldrive = device.as<Solid::OpticalDrive>();
        if (!opticaldrive) {
            return false;
        }

        devicetypes << I18N_NOOP("Optical Drive");

        QStringList supportedtypes;
        Solid::OpticalDrive::MediumTypes mediatypes = opticaldrive->supportedMedia();
        if (mediatypes & Solid::OpticalDrive::Cdr) {
            supportedtypes << I18N_NOOP("CD-R");
        }
        if (mediatypes & Solid::OpticalDrive::Cdrw) {
            supportedtypes << I18N_NOOP("CD-RW");
        }
        if (mediatypes & Solid::OpticalDrive::Dvd) {
            supportedtypes << I18N_NOOP("DVD");
        }
        if (mediatypes & Solid::OpticalDrive::Dvdr) {
            supportedtypes << I18N_NOOP("DVD-R");
        }
        if (mediatypes & Solid::OpticalDrive::Dvdrw) {
            supportedtypes << I18N_NOOP("DVD-RW");
        }
        if (mediatypes & Solid::OpticalDrive::Dvdram) {
            supportedtypes << I18N_NOOP("DVD-RAM");
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusr) {
            supportedtypes << I18N_NOOP("DVD+R");
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusrw) {
            supportedtypes << I18N_NOOP("DVD+RW");
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusdl) {
            supportedtypes << I18N_NOOP("DVD+DL");
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusdlrw) {
            supportedtypes << I18N_NOOP("DVD+DLRW");
        }
        if (mediatypes & Solid::OpticalDrive::Bd) {
            supportedtypes << I18N_NOOP("BD");
        }
        if (mediatypes & Solid::OpticalDrive::Bdr) {
            supportedtypes << I18N_NOOP("BD-R");
        }
        if (mediatypes & Solid::OpticalDrive::Bdre) {
            supportedtypes << I18N_NOOP("BD-RE");
        }
        if (mediatypes & Solid::OpticalDrive::HdDvd) {
            supportedtypes << I18N_NOOP("HDDVD");
        }
        if (mediatypes & Solid::OpticalDrive::HdDvdr) {
            supportedtypes << I18N_NOOP("HDDVD-R");
        }
        if (mediatypes & Solid::OpticalDrive::HdDvdrw) {
            supportedtypes << I18N_NOOP("HDDVD-RW");
        }
        setData(name, I18N_NOOP("Supported Media"), supportedtypes);

        setData(name, I18N_NOOP("Read Speed"), opticaldrive->readSpeed());
        setData(name, I18N_NOOP("Write Speed"), opticaldrive->writeSpeed());

        // the following method return QList<int> so we need to convert it to QList<QVariant>
        const QList<int> writespeeds = opticaldrive->writeSpeeds();
        QList<QVariant> variantlist;
        foreach (int num, writespeeds) {
            variantlist << num;
        }
        setData(name, I18N_NOOP("Write Speeds"), variantlist);
    }
    if (device.is<Solid::StorageVolume>()) {
        Solid::StorageVolume *storagevolume = device.as<Solid::StorageVolume>();
        if (!storagevolume) {
            return false;
        }

        devicetypes << I18N_NOOP("Storage Volume");

        QStringList usagetypes;
        usagetypes << i18n("Other") << i18n("Unused") << i18n("File System") << i18n("Partition Table") << i18n("Raid") << i18n("Encrypted");

        if (usagetypes.count() > storagevolume->usage()) {
            setData(name, I18N_NOOP("Usage"), usagetypes.at((int)storagevolume->usage()));
        } else {
            setData(name, I18N_NOOP("Usage"), i18n("Unknown"));
        }

        setData(name, I18N_NOOP("Ignored"), storagevolume->isIgnored());
        setData(name, I18N_NOOP("File System Type"), storagevolume->fsType());
        setData(name, I18N_NOOP("Label"), storagevolume->label());
        setData(name, I18N_NOOP("UUID"), storagevolume->uuid());
        updateInUse(name);

        // Check if the volume is part of an encrypted container
        // This needs to trigger an update for the encrypted container volume since
        // libsolid cannot notify us when the accessibility of the container changes
        Solid::Device encryptedContainer = storagevolume->encryptedContainer();
        if (encryptedContainer.isValid()) {
            const QString containerUdi = encryptedContainer.udi();
            setData(name, I18N_NOOP("Encrypted Container"), containerUdi);
            m_encryptedContainerMap[name] = containerUdi;
            // TODO: compress the calls?
            forceUpdateAccessibility(containerUdi);
        }
    }
    if (device.is<Solid::OpticalDisc>()) {
        Solid::OpticalDisc *opticaldisc = device.as<Solid::OpticalDisc>();
        if (!opticaldisc) {
            return false;
        }

        devicetypes << I18N_NOOP("OpticalDisc");

        // get the content types
        QStringList contenttypelist;
        const Solid::OpticalDisc::ContentTypes contenttypes = opticaldisc->availableContent();
        if (contenttypes.testFlag(Solid::OpticalDisc::Audio)) {
            contenttypelist << I18N_NOOP("Audio");
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::Data)) {
            contenttypelist << I18N_NOOP("Data");
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::VideoCd)) {
            contenttypelist << I18N_NOOP("Video CD");
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::SuperVideoCd)) {
            contenttypelist << I18N_NOOP("Super Video CD");
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::VideoDvd)) {
            contenttypelist << I18N_NOOP("Video DVD");
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::VideoBluRay)) {
            contenttypelist << I18N_NOOP("Video Blu Ray");
        }
        setData(name, I18N_NOOP("Available Content"), contenttypelist);

        QStringList disctypes;
        disctypes << I18N_NOOP("Unknown Disc Type") << I18N_NOOP("CD Rom") << I18N_NOOP("CD Recordable") << I18N_NOOP("CD Rewritable") << I18N_NOOP("DVD Rom")
                  << I18N_NOOP("DVD Ram") << I18N_NOOP("DVD Recordable") << I18N_NOOP("DVD Rewritable") << I18N_NOOP("DVD Plus Recordable")
                  << I18N_NOOP("DVD Plus Rewritable") << I18N_NOOP("DVD Plus Recordable Duallayer") << I18N_NOOP("DVD Plus Rewritable Duallayer")
                  << I18N_NOOP("Blu Ray Rom") << I18N_NOOP("Blu Ray Recordable") << I18N_NOOP("Blu Ray Rewritable") << I18N_NOOP("HD DVD Rom")
                  << I18N_NOOP("HD DVD Recordable") << I18N_NOOP("HD DVD Rewritable");
        //+1 because the enum starts at -1
        setData(name, I18N_NOOP("Disc Type"), disctypes.at((int)opticaldisc->discType() + 1));
        setData(name, I18N_NOOP("Appendable"), opticaldisc->isAppendable());
        setData(name, I18N_NOOP("Blank"), opticaldisc->isBlank());
        setData(name, I18N_NOOP("Rewritable"), opticaldisc->isRewritable());
        setData(name, I18N_NOOP("Capacity"), opticaldisc->capacity());
    }
    if (device.is<Solid::Camera>()) {
        Solid::Camera *camera = device.as<Solid::Camera>();
        if (!camera) {
            return false;
        }

        devicetypes << I18N_NOOP("Camera");

        setData(name, I18N_NOOP("Supported Protocols"), camera->supportedProtocols());
        setData(name, I18N_NOOP("Supported Drivers"), camera->supportedDrivers());
        // Cameras are necessarily Removable and Hotpluggable
        setData(name, I18N_NOOP("Removable"), true);
        setData(name, I18N_NOOP("Hotpluggable"), true);
    }
    if (device.is<Solid::PortableMediaPlayer>()) {
        Solid::PortableMediaPlayer *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (!mediaplayer) {
            return false;
        }

        devicetypes << I18N_NOOP("Portable Media Player");

        setData(name, I18N_NOOP("Supported Protocols"), mediaplayer->supportedProtocols());
        setData(name, I18N_NOOP("Supported Drivers"), mediaplayer->supportedDrivers());
        // Portable Media Players are necessarily Removable and Hotpluggable
        setData(name, I18N_NOOP("Removable"), true);
        setData(name, I18N_NOOP("Hotpluggable"), true);
    }
    if (device.is<Solid::Battery>()) {
        Solid::Battery *battery = device.as<Solid::Battery>();
        if (!battery) {
            return false;
        }

        devicetypes << I18N_NOOP("Battery");

        QStringList batterytype;
        batterytype << I18N_NOOP("Unknown Battery") << I18N_NOOP("PDA Battery") << I18N_NOOP("UPS Battery") << I18N_NOOP("Primary Battery")
                    << I18N_NOOP("Mouse Battery") << I18N_NOOP("Keyboard Battery") << I18N_NOOP("Keyboard Mouse Battery") << I18N_NOOP("Camera Battery")
                    << I18N_NOOP("Phone Battery") << I18N_NOOP("Monitor Battery") << I18N_NOOP("Gaming Input Battery") << I18N_NOOP("Bluetooth Battery");

        QStringList chargestate;
        chargestate << I18N_NOOP("Not Charging") << I18N_NOOP("Charging") << I18N_NOOP("Discharging") << I18N_NOOP("Fully Charged");

        setData(name, I18N_NOOP("Plugged In"), battery->isPresent()); // FIXME Rename when interested parties are adjusted
        setData(name, I18N_NOOP("Type"), batterytype.value((int)battery->type()));
        setData(name, I18N_NOOP("Charge Percent"), battery->chargePercent());
        setData(name, I18N_NOOP("Rechargeable"), battery->isRechargeable());
        setData(name, I18N_NOOP("Charge State"), chargestate.at((int)battery->chargeState()));

        m_signalmanager->mapDevice(battery, device.udi());
    }

    using namespace Solid;
    // we cannot just iterate the enum in reverse order since Battery comes second to last
    // and then our phone which also has a battery gets treated as battery :(
    static const Solid::DeviceInterface::Type typeOrder[] = {Solid::DeviceInterface::PortableMediaPlayer,
                                                             Solid::DeviceInterface::Camera,
                                                             Solid::DeviceInterface::OpticalDisc,
                                                             Solid::DeviceInterface::StorageVolume,
                                                             Solid::DeviceInterface::OpticalDrive,
                                                             Solid::DeviceInterface::StorageDrive,
                                                             Solid::DeviceInterface::NetworkShare,
                                                             Solid::DeviceInterface::StorageAccess,
                                                             Solid::DeviceInterface::Block,
                                                             Solid::DeviceInterface::Battery,
                                                             Solid::DeviceInterface::Processor};

    for (int i = 0; i < 11; ++i) {
        const Solid::DeviceInterface *interface = device.asDeviceInterface(typeOrder[i]);
        if (interface) {
            setData(name, I18N_NOOP("Type Description"), Solid::DeviceInterface::typeDescription(typeOrder[i]));
            break;
        }
    }

    setData(name, I18N_NOOP("Device Types"), devicetypes);
    return true;
}

void SolidDeviceEngine::deviceAdded(const QString &udi)
{
    Solid::Device device(udi);

    foreach (const QString &query, m_predicatemap.keys()) {
        Solid::Predicate predicate = Solid::Predicate::fromString(query);
        if (predicate.matches(device)) {
            m_predicatemap[query] << udi;
            setData(query, m_predicatemap[query]);
        }
    }

    if (device.is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
        if (drive) {
            connect(drive, &Solid::OpticalDrive::ejectRequested, this, &SolidDeviceEngine::setUnmountingState);
            connect(drive, &Solid::OpticalDrive::ejectDone, this, &SolidDeviceEngine::setIdleState);
        }
    } else if (device.is<Solid::StorageVolume>()) {
        // update the volume in case of 2-stage devices
        if (m_devicemap.contains(udi) && containerForSource(udi)->data().value(I18N_NOOP("Size")).toULongLong() == 0) {
            Solid::GenericInterface *iface = device.as<Solid::GenericInterface>();
            if (iface) {
                iface->setProperty("udi", udi);
                connect(iface, SIGNAL(propertyChanged(QMap<QString, int>)), this, SLOT(deviceChanged(QMap<QString, int>)));
            }
        }

        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            connect(access, &Solid::StorageAccess::setupRequested, this, &SolidDeviceEngine::setMountingState);
            connect(access, &Solid::StorageAccess::setupDone, this, &SolidDeviceEngine::setIdleState);
            connect(access, &Solid::StorageAccess::teardownRequested, this, &SolidDeviceEngine::setUnmountingState);
            connect(access, &Solid::StorageAccess::teardownDone, this, &SolidDeviceEngine::setIdleState);
        }
    }
}

void SolidDeviceEngine::setMountingState(const QString &udi)
{
    setData(udi, I18N_NOOP("State"), Mounting);
    setData(udi, I18N_NOOP("Operation result"), Working);
}

void SolidDeviceEngine::setUnmountingState(const QString &udi)
{
    setData(udi, I18N_NOOP("State"), Unmounting);
    setData(udi, I18N_NOOP("Operation result"), Working);
}

void SolidDeviceEngine::setIdleState(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
    Q_UNUSED(errorData)

    if (error == Solid::NoError) {
        setData(udi, I18N_NOOP("Operation result"), Successful);
    } else {
        setData(udi, I18N_NOOP("Operation result"), Unsuccessful);
    }
    setData(udi, I18N_NOOP("State"), Idle);

    Solid::Device device = m_devicemap.value(udi);
    if (!device.isValid()) {
        return;
    }

    Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
    if (!storageaccess) {
        return;
    }

    setData(udi, I18N_NOOP("Accessible"), storageaccess->isAccessible());
    setData(udi, I18N_NOOP("File Path"), storageaccess->filePath());
}

void SolidDeviceEngine::deviceChanged(const QMap<QString, int> &props)
{
    Solid::GenericInterface *iface = qobject_cast<Solid::GenericInterface *>(sender());
    if (iface && iface->isValid() && props.contains(QLatin1String("Size")) && iface->property(QStringLiteral("Size")).toInt() > 0) {
        const QString udi = qobject_cast<QObject *>(iface)->property("udi").toString();
        if (populateDeviceData(udi))
            forceImmediateUpdateOfAllVisualizations();
    }
}

bool SolidDeviceEngine::updateStorageSpace(const QString &udi)
{
    Solid::Device device = m_devicemap.value(udi);

    Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
    if (!storageaccess || !storageaccess->isAccessible()) {
        return false;
    }

    QString path = storageaccess->filePath();
    if (!m_paths.contains(path)) {
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, [path]() {
            KNotification::event(KNotification::Error, i18n("Filesystem is not responding"), i18n("Filesystem mounted at '%1' is not responding", path));
        });

        m_paths.insert(path);

        // create job
        KIO::FileSystemFreeSpaceJob *job = KIO::fileSystemFreeSpace(QUrl::fromLocalFile(path));

        // delete later timer
        connect(job, &KIO::FileSystemFreeSpaceJob::result, timer, &QTimer::deleteLater);

        // collect and process info
        connect(job, &KIO::FileSystemFreeSpaceJob::result, this, [this, timer, path, udi](KIO::Job *job, KIO::filesize_t size, KIO::filesize_t available) {
            timer->stop();

            if (!job->error()) {
                setData(udi, I18N_NOOP("Free Space"), QVariant(available).toDouble());
                setData(udi, I18N_NOOP("Free Space Text"), KFormat().formatByteSize(available));
                setData(udi, I18N_NOOP("Size"), QVariant(size).toDouble());
                setData(udi, I18N_NOOP("Size Text"), KFormat().formatByteSize(size));
            }

            m_paths.remove(path);
        });

        // start timer: after 15 seconds we will get an error
        timer->start(15000);
    }

    return false;
}

bool SolidDeviceEngine::updateHardDiskTemperature(const QString &udi)
{
    Solid::Device device = m_devicemap.value(udi);
    Solid::Block *block = device.as<Solid::Block>();
    if (!block) {
        return false;
    }

    if (!m_temperature) {
        m_temperature = new HddTemp(this);
    }

    if (m_temperature->sources().contains(block->device())) {
        setData(udi, I18N_NOOP("Temperature"), m_temperature->data(block->device(), HddTemp::Temperature));
        setData(udi, I18N_NOOP("Temperature Unit"), m_temperature->data(block->device(), HddTemp::Unit));
        return true;
    }

    return false;
}

bool SolidDeviceEngine::updateEmblems(const QString &udi)
{
    Solid::Device device = m_devicemap.value(udi);

    setData(udi, I18N_NOOP("Emblems"), device.emblems());
    return true;
}

bool SolidDeviceEngine::forceUpdateAccessibility(const QString &udi)
{
    Solid::Device device = m_devicemap.value(udi);
    if (!device.isValid()) {
        return false;
    }

    updateEmblems(udi);
    Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
    if (storageaccess) {
        setData(udi, I18N_NOOP("Accessible"), storageaccess->isAccessible());
    }

    return true;
}

bool SolidDeviceEngine::updateInUse(const QString &udi)
{
    Solid::Device device = m_devicemap.value(udi);
    if (!device.isValid()) {
        return false;
    }

    Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
    if (!storageaccess) {
        return false;
    }

    if (storageaccess->isAccessible()) {
        setData(udi, I18N_NOOP("In Use"), true);
    } else {
        Solid::StorageDrive *drive = getAncestorAs<Solid::StorageDrive>(Solid::Device(udi));
        if (drive) {
            setData(udi, I18N_NOOP("In Use"), drive->isInUse());
        }
    }

    return true;
}

bool SolidDeviceEngine::updateSourceEvent(const QString &source)
{
    bool update1 = updateStorageSpace(source);
    bool update2 = updateHardDiskTemperature(source);
    bool update3 = updateEmblems(source);
    bool update4 = updateInUse(source);

    return (update1 || update2 || update3 || update4);
}

void SolidDeviceEngine::deviceRemoved(const QString &udi)
{
    // libsolid cannot notify us when an encrypted container is closed,
    // hence we trigger an update when a device contained in an encrypted container device dies
    const QString containerUdi = m_encryptedContainerMap.value(udi, QString());

    if (!containerUdi.isEmpty()) {
        forceUpdateAccessibility(containerUdi);
        m_encryptedContainerMap.remove(udi);
    }

    foreach (const QString &query, m_predicatemap.keys()) {
        m_predicatemap[query].removeAll(udi);
        setData(query, m_predicatemap[query]);
    }

    Solid::Device device(udi);
    if (device.is<Solid::StorageVolume>()) {
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            disconnect(access, nullptr, this, nullptr);
        }
    } else if (device.is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
        if (drive) {
            disconnect(drive, nullptr, this, nullptr);
        }
    }

    m_devicemap.remove(udi);
    removeSource(udi);
}

void SolidDeviceEngine::deviceChanged(const QString &udi, const QString &property, const QVariant &value)
{
    setData(udi, property, value);
    updateSourceEvent(udi);
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(soliddevice, SolidDeviceEngine, "plasma-dataengine-soliddevice.json")

#include "soliddeviceengine.moc"
