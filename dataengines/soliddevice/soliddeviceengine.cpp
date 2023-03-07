/*
    SPDX-FileCopyrightText: 2007 Christopher Blauvelt <cblauvelt@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "soliddeviceengine.h"
#include "soliddeviceservice.h"

#include <KLazyLocalizedString>
#include <QDateTime>
#include <QMetaEnum>
#include <Solid/GenericInterface>
#include <klocalizedstring.h>

#include <KFormat>
#include <KNotification>
#include <QApplication>
#include <QDebug>

#include <Plasma5Support/DataContainer>

// TODO: implement in libsolid2
namespace
{
template<class DevIface>
DevIface *getAncestorAs(const Solid::Device &device)
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
    : Plasma5Support::DataEngine(parent, args)
    , m_temperature(nullptr)
    , m_notifier(nullptr)
{
    Q_UNUSED(args)
    m_signalmanager = new DeviceSignalMapManager(this);

    listenForNewDevices();
    setMinimumPollingInterval(1000);
    connect(this, &Plasma5Support::DataEngine::sourceRemoved, this, &SolidDeviceEngine::sourceWasRemoved);
}

SolidDeviceEngine::~SolidDeviceEngine()
{
}

Plasma5Support::Service *SolidDeviceEngine::serviceForSource(const QString &source)
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
    setData(name, kli18n("Parent UDI").untranslatedText(), device.parentUdi());
    setData(name, kli18n("Vendor").untranslatedText(), device.vendor());
    setData(name, kli18n("Product").untranslatedText(), device.product());
    setData(name, kli18n("Description").untranslatedText(), device.description());
    setData(name, kli18n("Icon").untranslatedText(), device.icon());
    setData(name, kli18n("Emblems").untranslatedText(), device.emblems());
    setData(name, kli18n("State").untranslatedText(), Idle);
    setData(name, kli18n("Operation result").untranslatedText(), Working);
    setData(name, kli18n("Timestamp").untranslatedText(), QDateTime::currentDateTimeUtc());

    if (device.is<Solid::Processor>()) {
        Solid::Processor *processor = device.as<Solid::Processor>();
        if (!processor) {
            return false;
        }

        devicetypes << kli18n("Processor").untranslatedText();
        setData(name, kli18n("Number").untranslatedText(), processor->number());
        setData(name, kli18n("Max Speed").untranslatedText(), processor->maxSpeed());
        setData(name, kli18n("Can Change Frequency").untranslatedText(), processor->canChangeFrequency());
    }
    if (device.is<Solid::Block>()) {
        Solid::Block *block = device.as<Solid::Block>();
        if (!block) {
            return false;
        }

        devicetypes << kli18n("Block").untranslatedText();
        setData(name, kli18n("Major").untranslatedText(), block->deviceMajor());
        setData(name, kli18n("Minor").untranslatedText(), block->deviceMinor());
        setData(name, kli18n("Device").untranslatedText(), block->device());
    }
    if (device.is<Solid::StorageAccess>()) {
        Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
        if (!storageaccess) {
            return false;
        }

        devicetypes << kli18n("Storage Access").untranslatedText();
        setData(name, kli18n("Accessible").untranslatedText(), storageaccess->isAccessible());
        setData(name, kli18n("File Path").untranslatedText(), storageaccess->filePath());

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

        devicetypes << kli18n("Storage Drive").untranslatedText();

        QStringList bus;
        bus << kli18n("Ide").untranslatedText() << kli18n("Usb").untranslatedText() << kli18n("Ieee1394").untranslatedText()
            << kli18n("Scsi").untranslatedText() << kli18n("Sata").untranslatedText() << kli18n("Platform").untranslatedText();
        QStringList drivetype;
        drivetype << kli18n("Hard Disk").untranslatedText() << kli18n("Cdrom Drive").untranslatedText() << kli18n("Floppy").untranslatedText()
                  << kli18n("Tape").untranslatedText() << kli18n("Compact Flash").untranslatedText() << kli18n("Memory Stick").untranslatedText()
                  << kli18n("Smart Media").untranslatedText() << kli18n("SdMmc").untranslatedText() << kli18n("Xd").untranslatedText();

        setData(name, kli18n("Bus").untranslatedText(), bus.at((int)storagedrive->bus()));
        setData(name, kli18n("Drive Type").untranslatedText(), drivetype.at((int)storagedrive->driveType()));
        setData(name, kli18n("Removable").untranslatedText(), storagedrive->isRemovable());
        setData(name, kli18n("Hotpluggable").untranslatedText(), storagedrive->isHotpluggable());

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
        setData(name, kli18n("Removable").untranslatedText(), isRemovable);
        setData(name, kli18n("Hotpluggable").untranslatedText(), isHotpluggable);
    }

    if (device.is<Solid::OpticalDrive>()) {
        Solid::OpticalDrive *opticaldrive = device.as<Solid::OpticalDrive>();
        if (!opticaldrive) {
            return false;
        }

        devicetypes << kli18n("Optical Drive").untranslatedText();

        QStringList supportedtypes;
        Solid::OpticalDrive::MediumTypes mediatypes = opticaldrive->supportedMedia();
        if (mediatypes & Solid::OpticalDrive::Cdr) {
            supportedtypes << kli18n("CD-R").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Cdrw) {
            supportedtypes << kli18n("CD-RW").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvd) {
            supportedtypes << kli18n("DVD").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvdr) {
            supportedtypes << kli18n("DVD-R").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvdrw) {
            supportedtypes << kli18n("DVD-RW").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvdram) {
            supportedtypes << kli18n("DVD-RAM").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusr) {
            supportedtypes << kli18n("DVD+R").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusrw) {
            supportedtypes << kli18n("DVD+RW").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusdl) {
            supportedtypes << kli18n("DVD+DL").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Dvdplusdlrw) {
            supportedtypes << kli18n("DVD+DLRW").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Bd) {
            supportedtypes << kli18n("BD").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Bdr) {
            supportedtypes << kli18n("BD-R").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::Bdre) {
            supportedtypes << kli18n("BD-RE").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::HdDvd) {
            supportedtypes << kli18n("HDDVD").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::HdDvdr) {
            supportedtypes << kli18n("HDDVD-R").untranslatedText();
        }
        if (mediatypes & Solid::OpticalDrive::HdDvdrw) {
            supportedtypes << kli18n("HDDVD-RW").untranslatedText();
        }
        setData(name, kli18n("Supported Media").untranslatedText(), supportedtypes);

        setData(name, kli18n("Read Speed").untranslatedText(), opticaldrive->readSpeed());
        setData(name, kli18n("Write Speed").untranslatedText(), opticaldrive->writeSpeed());

        // the following method return QList<int> so we need to convert it to QList<QVariant>
        const QList<int> writespeeds = opticaldrive->writeSpeeds();
        QList<QVariant> variantlist;
        foreach (int num, writespeeds) {
            variantlist << num;
        }
        setData(name, kli18n("Write Speeds").untranslatedText(), variantlist);
    }
    if (device.is<Solid::StorageVolume>()) {
        Solid::StorageVolume *storagevolume = device.as<Solid::StorageVolume>();
        if (!storagevolume) {
            return false;
        }

        devicetypes << kli18n("Storage Volume").untranslatedText();

        QStringList usagetypes;
        usagetypes << i18n("Other") << i18n("Unused") << i18n("File System") << i18n("Partition Table") << i18n("Raid") << i18n("Encrypted");

        if (usagetypes.count() > storagevolume->usage()) {
            setData(name, kli18n("Usage").untranslatedText(), usagetypes.at((int)storagevolume->usage()));
        } else {
            setData(name, kli18n("Usage").untranslatedText(), i18n("Unknown"));
        }

        setData(name, kli18n("Ignored").untranslatedText(), storagevolume->isIgnored());
        setData(name, kli18n("File System Type").untranslatedText(), storagevolume->fsType());
        setData(name, kli18n("Label").untranslatedText(), storagevolume->label());
        setData(name, kli18n("UUID").untranslatedText(), storagevolume->uuid());
        updateInUse(name);

        // Check if the volume is part of an encrypted container
        // This needs to trigger an update for the encrypted container volume since
        // libsolid cannot notify us when the accessibility of the container changes
        Solid::Device encryptedContainer = storagevolume->encryptedContainer();
        if (encryptedContainer.isValid()) {
            const QString containerUdi = encryptedContainer.udi();
            setData(name, kli18n("Encrypted Container").untranslatedText(), containerUdi);
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

        devicetypes << kli18n("OpticalDisc").untranslatedText();

        // get the content types
        QStringList contenttypelist;
        const Solid::OpticalDisc::ContentTypes contenttypes = opticaldisc->availableContent();
        if (contenttypes.testFlag(Solid::OpticalDisc::Audio)) {
            contenttypelist << kli18n("Audio").untranslatedText();
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::Data)) {
            contenttypelist << kli18n("Data").untranslatedText();
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::VideoCd)) {
            contenttypelist << kli18n("Video CD").untranslatedText();
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::SuperVideoCd)) {
            contenttypelist << kli18n("Super Video CD").untranslatedText();
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::VideoDvd)) {
            contenttypelist << kli18n("Video DVD").untranslatedText();
        }
        if (contenttypes.testFlag(Solid::OpticalDisc::VideoBluRay)) {
            contenttypelist << kli18n("Video Blu Ray").untranslatedText();
        }
        setData(name, kli18n("Available Content").untranslatedText(), contenttypelist);

        QStringList disctypes;
        disctypes << kli18n("Unknown Disc Type").untranslatedText() << kli18n("CD Rom").untranslatedText() << kli18n("CD Recordable").untranslatedText()
                  << kli18n("CD Rewritable").untranslatedText() << kli18n("DVD Rom").untranslatedText() << kli18n("DVD Ram").untranslatedText()
                  << kli18n("DVD Recordable").untranslatedText() << kli18n("DVD Rewritable").untranslatedText()
                  << kli18n("DVD Plus Recordable").untranslatedText() << kli18n("DVD Plus Rewritable").untranslatedText()
                  << kli18n("DVD Plus Recordable Duallayer").untranslatedText() << kli18n("DVD Plus Rewritable Duallayer").untranslatedText()
                  << kli18n("Blu Ray Rom").untranslatedText() << kli18n("Blu Ray Recordable").untranslatedText()
                  << kli18n("Blu Ray Rewritable").untranslatedText() << kli18n("HD DVD Rom").untranslatedText()
                  << kli18n("HD DVD Recordable").untranslatedText() << kli18n("HD DVD Rewritable").untranslatedText();
        //+1 because the enum starts at -1
        setData(name, kli18n("Disc Type").untranslatedText(), disctypes.at((int)opticaldisc->discType() + 1));
        setData(name, kli18n("Appendable").untranslatedText(), opticaldisc->isAppendable());
        setData(name, kli18n("Blank").untranslatedText(), opticaldisc->isBlank());
        setData(name, kli18n("Rewritable").untranslatedText(), opticaldisc->isRewritable());
        setData(name, kli18n("Capacity").untranslatedText(), opticaldisc->capacity());
    }
    if (device.is<Solid::Camera>()) {
        Solid::Camera *camera = device.as<Solid::Camera>();
        if (!camera) {
            return false;
        }

        devicetypes << kli18n("Camera").untranslatedText();

        setData(name, kli18n("Supported Protocols").untranslatedText(), camera->supportedProtocols());
        setData(name, kli18n("Supported Drivers").untranslatedText(), camera->supportedDrivers());
        // Cameras are necessarily Removable and Hotpluggable
        setData(name, kli18n("Removable").untranslatedText(), true);
        setData(name, kli18n("Hotpluggable").untranslatedText(), true);
    }
    if (device.is<Solid::PortableMediaPlayer>()) {
        Solid::PortableMediaPlayer *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (!mediaplayer) {
            return false;
        }

        devicetypes << kli18n("Portable Media Player").untranslatedText();

        setData(name, kli18n("Supported Protocols").untranslatedText(), mediaplayer->supportedProtocols());
        setData(name, kli18n("Supported Drivers").untranslatedText(), mediaplayer->supportedDrivers());
        // Portable Media Players are necessarily Removable and Hotpluggable
        setData(name, kli18n("Removable").untranslatedText(), true);
        setData(name, kli18n("Hotpluggable").untranslatedText(), true);
    }
    if (device.is<Solid::Battery>()) {
        Solid::Battery *battery = device.as<Solid::Battery>();
        if (!battery) {
            return false;
        }

        devicetypes << kli18n("Battery").untranslatedText();

        QStringList batterytype;
        batterytype << kli18n("Unknown Battery").untranslatedText() << kli18n("PDA Battery").untranslatedText() << kli18n("UPS Battery").untranslatedText()
                    << kli18n("Primary Battery").untranslatedText() << kli18n("Mouse Battery").untranslatedText()
                    << kli18n("Keyboard Battery").untranslatedText() << kli18n("Keyboard Mouse Battery").untranslatedText()
                    << kli18n("Camera Battery").untranslatedText() << kli18n("Phone Battery").untranslatedText() << kli18n("Monitor Battery").untranslatedText()
                    << kli18n("Gaming Input Battery").untranslatedText() << kli18n("Bluetooth Battery").untranslatedText();

        QStringList chargestate;
        chargestate << kli18n("Not Charging").untranslatedText() << kli18n("Charging").untranslatedText() << kli18n("Discharging").untranslatedText()
                    << kli18n("Fully Charged").untranslatedText();

        setData(name, kli18n("Plugged In").untranslatedText(), battery->isPresent()); // FIXME Rename when interested parties are adjusted
        setData(name, kli18n("Type").untranslatedText(), batterytype.value((int)battery->type()));
        setData(name, kli18n("Charge Percent").untranslatedText(), battery->chargePercent());
        setData(name, kli18n("Rechargeable").untranslatedText(), battery->isRechargeable());
        setData(name, kli18n("Charge State").untranslatedText(), chargestate.at((int)battery->chargeState()));

        m_signalmanager->mapDevice(battery, device.udi());
    }

    using namespace Solid;
    // we cannot just iterate the enum in reverse order since Battery comes second to last
    // and then our phone which also has a battery gets treated as battery :(
    static const Solid::DeviceInterface::Type typeOrder[] = {
        Solid::DeviceInterface::PortableMediaPlayer,
        Solid::DeviceInterface::Camera,
        Solid::DeviceInterface::OpticalDisc,
        Solid::DeviceInterface::StorageVolume,
        Solid::DeviceInterface::OpticalDrive,
        Solid::DeviceInterface::StorageDrive,
        Solid::DeviceInterface::NetworkShare,
        Solid::DeviceInterface::StorageAccess,
        Solid::DeviceInterface::Block,
        Solid::DeviceInterface::Battery,
        Solid::DeviceInterface::Processor,
    };

    for (int i = 0; i < 11; ++i) {
        const Solid::DeviceInterface *interface = device.asDeviceInterface(typeOrder[i]);
        if (interface) {
            setData(name, kli18n("Type Description").untranslatedText(), Solid::DeviceInterface::typeDescription(typeOrder[i]));
            break;
        }
    }

    setData(name, kli18n("Device Types").untranslatedText(), devicetypes);
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
        if (m_devicemap.contains(udi) && containerForSource(udi)->data().value(kli18n("Size").untranslatedText()).toULongLong() == 0) {
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
    setData(udi, kli18n("State").untranslatedText(), Mounting);
    setData(udi, kli18n("Operation result").untranslatedText(), Working);
}

void SolidDeviceEngine::setUnmountingState(const QString &udi)
{
    setData(udi, kli18n("State").untranslatedText(), Unmounting);
    setData(udi, kli18n("Operation result").untranslatedText(), Working);
}

void SolidDeviceEngine::setIdleState(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
    Q_UNUSED(errorData)

    if (error == Solid::NoError) {
        setData(udi, kli18n("Operation result").untranslatedText(), Successful);
    } else {
        setData(udi, kli18n("Operation result").untranslatedText(), Unsuccessful);
    }
    setData(udi, kli18n("State").untranslatedText(), Idle);

    Solid::Device device = m_devicemap.value(udi);
    if (!device.isValid()) {
        return;
    }

    Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
    if (!storageaccess) {
        return;
    }

    setData(udi, kli18n("Accessible").untranslatedText(), storageaccess->isAccessible());
    setData(udi, kli18n("File Path").untranslatedText(), storageaccess->filePath());
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
                setData(udi, kli18n("Free Space").untranslatedText(), QVariant(available).toDouble());
                setData(udi, kli18n("Free Space Text").untranslatedText(), KFormat().formatByteSize(available));
                setData(udi, kli18n("Size").untranslatedText(), QVariant(size).toDouble());
                setData(udi, kli18n("Size Text").untranslatedText(), KFormat().formatByteSize(size));
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
        setData(udi, kli18n("Temperature").untranslatedText(), m_temperature->data(block->device(), HddTemp::Temperature));
        setData(udi, kli18n("Temperature Unit").untranslatedText(), m_temperature->data(block->device(), HddTemp::Unit));
        return true;
    }

    return false;
}

bool SolidDeviceEngine::updateEmblems(const QString &udi)
{
    Solid::Device device = m_devicemap.value(udi);

    setData(udi, kli18n("Emblems").untranslatedText(), device.emblems());
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
        setData(udi, kli18n("Accessible").untranslatedText(), storageaccess->isAccessible());
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
        setData(udi, kli18n("In Use").untranslatedText(), true);
    } else {
        Solid::StorageDrive *drive = getAncestorAs<Solid::StorageDrive>(Solid::Device(udi));
        if (drive) {
            setData(udi, kli18n("In Use").untranslatedText(), drive->isInUse());
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

K_PLUGIN_CLASS_WITH_JSON(SolidDeviceEngine, "plasma-dataengine-soliddevice.json")

#include "soliddeviceengine.moc"
