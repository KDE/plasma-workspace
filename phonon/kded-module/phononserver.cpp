/*
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2013 Harald Sitter <sitter@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "phononserver.h"
#include "deviceinfo.h"

#include <kconfiggroup.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kdialog.h>
#include <KPluginFactory>
#include <KPluginLoader>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QRegExp>
#include <QtCore/QSettings>
#include <QtCore/QTimerEvent>
#include <QtCore/QProcess>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusMetaType>
#include <QtCore/QVariant>
#include <phonon/pulsesupport.h>
#include <Solid/AudioInterface>
#include <Solid/GenericInterface>
#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/Block>
#include <solid/video.h>    // FIXME to Solid/Video, when it appears

#include <../config-alsa.h>
#ifdef HAVE_LIBASOUND2
#include <alsa/asoundlib.h>
#endif // HAVE_LIBASOUND2

K_PLUGIN_FACTORY(PhononServerFactory, registerPlugin<PhononServer>(); )
K_EXPORT_PLUGIN(PhononServerFactory("phononserver"))

using namespace Phonon;


PhononServer::PhononServer(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent),
    m_config(KSharedConfig::openConfig("phonondevicesrc", KConfig::SimpleConfig))
{
    findDevices();
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), SLOT(deviceAdded(QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), SLOT(deviceRemoved(QString)));
}

PhononServer::~PhononServer()
{
}

static QString uniqueId(const Solid::Device &device, int deviceNumber)
{
    Q_UNUSED(deviceNumber);
    return device.udi();
#if 0
    const Solid::GenericInterface *genericIface = device.as<Solid::GenericInterface>();
    Q_ASSERT(genericIface);
    const QString &subsystem = genericIface->propertyExists(QLatin1String("info.subsystem")) ?
        genericIface->property(QLatin1String("info.subsystem")).toString() :
        genericIface->property(QLatin1String("linux.subsystem")).toString();
    if (subsystem == "pci") {
        const QVariant vendor_id = genericIface->property("pci.vendor_id");
        if (vendor_id.isValid()) {
            const QVariant product_id = genericIface->property("pci.product_id");
            if (product_id.isValid()) {
                const QVariant subsys_vendor_id = genericIface->property("pci.subsys_vendor_id");
                if (subsys_vendor_id.isValid()) {
                    const QVariant subsys_product_id = genericIface->property("pci.subsys_product_id");
                    if (subsys_product_id.isValid()) {
                        return QString("pci:%1:%2:%3:%4:%5")
                            .arg(vendor_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(product_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(subsys_vendor_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(subsys_product_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(deviceNumber);
                    }
                }
            }
        }
    } else if (subsystem == "usb" || subsystem == "usb_device") {
        const QVariant vendor_id = genericIface->property("usb.vendor_id");
        if (vendor_id.isValid()) {
            const QVariant product_id = genericIface->property("usb.product_id");
            if (product_id.isValid()) {
                return QString("usb:%1:%2:%3")
                    .arg(vendor_id.toInt(), 4, 16, QLatin1Char('0'))
                    .arg(product_id.toInt(), 4, 16, QLatin1Char('0'))
                    .arg(deviceNumber);
            }
        }
    } else {
        // not the right device, need to look at the parent (but not at the top-level device in the
        // device tree - that would be too far up the hierarchy)
        if (device.parent().isValid() && device.parent().parent().isValid()) {
            return uniqueId(device.parent(), deviceNumber);
        }
    }
    return QString();
#endif
}

static void renameDevices(QList<PS::DeviceInfo> *devicelist)
{
    QHash<QString, int> cardNames;
    foreach (const PS::DeviceInfo &dev, *devicelist) {
        ++cardNames[dev.name()];
    }

    // Now look for duplicate names and rename those by appending the device number
    QMutableListIterator<PS::DeviceInfo> it(*devicelist);
    while (it.hasNext()) {
        PS::DeviceInfo &dev = it.next();
        if (dev.deviceNumber() > 0 && cardNames.value(dev.name()) > 1) {
            dev.setPreferredName(dev.name() + QLatin1String(" #") + QString::number(dev.deviceNumber()));
        }
    }
}

struct DeviceHint
{
    QString name;
    QString description;
};

static inline QDebug operator<<(QDebug &d, const DeviceHint &h)
{
    d.nospace() << h.name << " (" << h.description << ")";
    return d;
}

void PhononServer::findVirtualDevices()
{
#ifdef HAVE_LIBASOUND2
    QList<DeviceHint> deviceHints;
    QHash<PS::DeviceKey, PS::DeviceInfo> playbackDevices;
    QHash<PS::DeviceKey, PS::DeviceInfo> captureDevices;

    // update config to the changes on disc
    snd_config_update_free_global();
    snd_config_update();

    void **hints;
    //snd_config_update();
    if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
        kDebug(601) << "snd_device_name_hint failed for 'pcm'";
        return;
    }

    for (void **cStrings = hints; *cStrings; ++cStrings) {
        DeviceHint nextHint;
        char *x = snd_device_name_get_hint(*cStrings, "NAME");
        nextHint.name = QString::fromUtf8(x);
        free(x);

        if (nextHint.name.isEmpty() || nextHint.name == "null")
            continue;

        if (nextHint.name.startsWith(QLatin1String("front:")) ||
            nextHint.name.startsWith(QLatin1String("rear:")) ||
            nextHint.name.startsWith(QLatin1String("center_lfe:")) ||
            nextHint.name.startsWith(QLatin1String("surround40:")) ||
            nextHint.name.startsWith(QLatin1String("surround41:")) ||
            nextHint.name.startsWith(QLatin1String("surround50:")) ||
            nextHint.name.startsWith(QLatin1String("surround51:")) ||
            nextHint.name.startsWith(QLatin1String("surround71:"))) {
            continue;
        }

        x = snd_device_name_get_hint(*cStrings, "DESC");
        nextHint.description = QString::fromUtf8(x);
        free(x);

        deviceHints << nextHint;
    }
    snd_device_name_free_hint(hints);
    kDebug(601) << deviceHints;

    snd_config_update_free_global();
    snd_config_update();
    Q_ASSERT(snd_config);
    foreach (const DeviceHint &deviceHint, deviceHints) {
        const QString &alsaDeviceName = deviceHint.name;
        const QString &description = deviceHint.description;
        QString uniqueId = description;
        //const QString &udi = alsaDeviceName;
        bool isAdvanced = false;

        // Try to determine the card name
        const QStringList &lines = description.split('\n');
        QString cardName = lines.first();

        if (cardName.isEmpty()) {
            int cardNameStart = alsaDeviceName.indexOf("CARD=");
            int cardNameEnd;

            if (cardNameStart >= 0) {
                cardNameStart += 5;
                cardNameEnd = alsaDeviceName.indexOf(',', cardNameStart);
                if (cardNameEnd < 0)
                    cardNameEnd = alsaDeviceName.count();

                cardName = alsaDeviceName.mid(cardNameStart, cardNameEnd - cardNameStart);
            } else {
                cardName = i18nc("unknown sound card", "Unknown");
            }
        }

        // Put a non-empty unique id if the description is empty
        if (uniqueId.isEmpty()) {
            uniqueId = cardName;
        }

        // Add a description to the card name, if it exists
        if (lines.size() > 1) {
            cardName = i18nc("%1 is the sound card name, %2 is the description in case it exists", "%1 (%2)", cardName, lines[1]);
        }

        bool playbackDevice = false;
        bool captureDevice = false;
        {
            snd_pcm_t *pcm;
            const QByteArray &deviceNameEnc = alsaDeviceName.toUtf8();
            if (0 == snd_pcm_open(&pcm, deviceNameEnc.constData(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK /*open mode: non-blocking, sync */)) {
                playbackDevice = true;
                snd_pcm_close(pcm);
            }
            if (0 == snd_pcm_open(&pcm, deviceNameEnc.constData(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK /*open mode: non-blocking, sync */)) {
                captureDevice = true;
                snd_pcm_close(pcm);
            }
        }

        if (alsaDeviceName.startsWith(QLatin1String("front:")) ||
            alsaDeviceName.startsWith(QLatin1String("rear:")) ||
            alsaDeviceName.startsWith(QLatin1String("center_lfe:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround40:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround41:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround50:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround51:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround71:")) ||
            alsaDeviceName.startsWith(QLatin1String("iec958:"))) {
            isAdvanced = true;
        }

        if (alsaDeviceName.startsWith(QLatin1String("front")) ||
            alsaDeviceName.startsWith(QLatin1String("center")) ||
            alsaDeviceName.startsWith(QLatin1String("rear")) ||
            alsaDeviceName.startsWith(QLatin1String("surround"))) {
            captureDevice = false;
        }

        QString iconName(QLatin1String("audio-card"));
        int initialPreference = 30;
        if (description.contains("headset", Qt::CaseInsensitive) ||
                description.contains("headphone", Qt::CaseInsensitive)) {
            // it's a headset
            if (description.contains("usb", Qt::CaseInsensitive)) {
                iconName = QLatin1String("audio-headset-usb");
                initialPreference -= 10;
            } else {
                iconName = QLatin1String("audio-headset");
                initialPreference -= 10;
            }
        } else {
            if (description.contains("usb", Qt::CaseInsensitive)) {
                // it's an external USB device
                iconName = QLatin1String("audio-card-usb");
                initialPreference -= 10;
            }
        }

        const PS::DeviceAccess access(QStringList(alsaDeviceName), 0, PS::DeviceAccess::AlsaDriver,
                captureDevice, playbackDevice);
        //dev.setUseCache(false);
        if (playbackDevice) {
            const PS::DeviceKey key = { uniqueId + QLatin1String("_playback"), -1, -1 };

            if (playbackDevices.contains(key)) {
                playbackDevices[key].addAccess(access);
            } else {
                PS::DeviceInfo dev(PS::DeviceInfo::Audio, cardName, iconName, key, initialPreference, isAdvanced);
                dev.addAccess(access);
                playbackDevices.insert(key, dev);
            }
        }
        if (captureDevice) {
            const PS::DeviceKey key = { uniqueId + QLatin1String("_capture"), -1, -1 };

            if (captureDevices.contains(key)) {
                captureDevices[key].addAccess(access);
            } else {
                PS::DeviceInfo dev(PS::DeviceInfo::Audio, cardName, iconName, key, initialPreference, isAdvanced);
                dev.addAccess(access);
                captureDevices.insert(key, dev);
            }
        } else {
            if (!playbackDevice) {
                kDebug(601) << deviceHint.name << " doesn't work.";
            }
        }
    }

    m_audioOutputDevices << playbackDevices.values();
    m_audioCaptureDevices << captureDevices.values();

    const QString etcFile(QLatin1String("/etc/asound.conf"));
    const QString homeFile(QDir::homePath() + QLatin1String("/.asoundrc"));
    const bool etcExists = QFile::exists(etcFile);
    const bool homeExists = QFile::exists(homeFile);
    if (etcExists || homeExists) {
        static QFileSystemWatcher *watcher = 0;
        if (!watcher) {
            watcher = new QFileSystemWatcher(this);
            connect(watcher, SIGNAL(fileChanged(QString)), SLOT(alsaConfigChanged()));
        }
        // QFileSystemWatcher stops monitoring after a file got removed. Many editors save files by
        // writing to a temp file and moving it over the other one. QFileSystemWatcher seems to
        // interpret that as removing and stops watching a file after it got modified by an editor.
        if (etcExists && !watcher->files().contains(etcFile)) {
            kDebug(601) << "setup QFileSystemWatcher for" << etcFile;
            watcher->addPath(etcFile);
        }
        if (homeExists && !watcher->files().contains(homeFile)) {
            kDebug(601) << "setup QFileSystemWatcher for" << homeFile;
            watcher->addPath(homeFile);
        }
    }
#endif // HAVE_LIBASOUND2
}

void PhononServer::alsaConfigChanged()
{
    kDebug(601);
    m_updateDevicesTimer.start(50, this);
}

static void removeOssOnlyDevices(QList<PS::DeviceInfo> *list)
{
    QMutableListIterator<PS::DeviceInfo> it(*list);
    while (it.hasNext()) {
        const PS::DeviceInfo &dev = it.next();
        if (dev.isAvailable()) {
            bool onlyOss = true;
            foreach (const PS::DeviceAccess &a, dev.accessList()) {
                if (a.driver() != PS::DeviceAccess::OssDriver) {
                    onlyOss = false;
                    break;
                }
            }
            if (onlyOss) {
                it.remove();
            }
        }
    }
}

void PhononServer::findDevices()
{
    if (Phonon::PulseSupport *pulse = Phonon::PulseSupport::getInstance()) {
        // NOTE: This is relying on internal behavior....
        //       enable internally simply sets a bool that is later && with the
        //       actually PA activity.
        //       Should this function ever start doing more, this will break horribly.
        pulse->enable();
        if (pulse->isActive()) {
            kDebug(601) << "Not looking for devices as Phonon::PulseSupport is active.";
            return;
        }
    }

    // Fetch the full list of audio and video devices from Solid
    const QList<Solid::Device> &solidAudioDevices =
        Solid::Device::listFromQuery("AudioInterface.deviceType & 'AudioInput|AudioOutput'");
    const QList<Solid::Device> &solidVideoDevices =
        Solid::Device::listFromType(Solid::DeviceInterface::Video);

    kDebug(601) << "Solid offers" << solidAudioDevices.count() << "audio devices";
    kDebug(601) << "Solid offers" << solidVideoDevices.count() << "video devices";

    // Collections of PhononServer devices, to be extracted from the ones from Solid
    QHash<PS::DeviceKey, PS::DeviceInfo> audioPlaybackDevices;
    QHash<PS::DeviceKey, PS::DeviceInfo> audioCaptureDevices;
    QHash<PS::DeviceKey, PS::DeviceInfo> videoCaptureDevices;

    QHash<QString, QList<int> > listOfCardNumsPerUniqueId;
    QStringList deviceIds;
    int accessPreference;
    PS::DeviceAccess::DeviceDriverType driver;
    bool valid = true;
    bool isAdvanced = false;
    bool preferCardName = false;

    /*
     * Process audio devices
     */
    bool haveAlsaDevices = false;
    foreach (const Solid::Device &hwDevice, solidAudioDevices) {
        const Solid::AudioInterface *audioIface = hwDevice.as<Solid::AudioInterface>();

        deviceIds.clear();
        accessPreference = 0;
        driver = PS::DeviceAccess::InvalidDriver;
        valid = true;
        isAdvanced = false;
        preferCardName = false;
        int cardNum = -1;
        int deviceNum = -1;

        kDebug(601) << "looking at device:" << audioIface->name() << audioIface->driverHandle();

        bool capture = audioIface->deviceType() & Solid::AudioInterface::AudioInput;
        bool playback = audioIface->deviceType() & Solid::AudioInterface::AudioOutput;

        switch (audioIface->driver()) {
        case Solid::AudioInterface::UnknownAudioDriver:
            valid = false;
            break;

        case Solid::AudioInterface::Alsa:
            if (audioIface->driverHandle().type() != QVariant::List) {
                valid = false;
            } else {
                haveAlsaDevices = true;
                // ALSA has better naming of the device than the corresponding OSS entry in HAL
                preferCardName = true;

                const QList<QVariant> handles = audioIface->driverHandle().toList();
                if (handles.size() < 1) {
                    valid = false;
                } else {
                    driver = PS::DeviceAccess::AlsaDriver;
                    accessPreference += 10;

                    bool ok;
                    cardNum = handles.first().toInt(&ok);
                    if (!ok) {
                        cardNum = -1;
                    }
                    const QString &cardStr = handles.first().toString();
                    // the first is either an int (card number) or a QString (card id)
                    QString x_phononId = QLatin1String("x-phonon:CARD=") + cardStr;
                    QString fallbackId = QLatin1String("plughw:CARD=") + cardStr;
                    if (handles.size() > 1 && handles.at(1).isValid()) {
                        deviceNum = handles.at(1).toInt();
                        const QString deviceStr = handles.at(1).toString();
                        if (deviceNum == 0) {
                            // prefer DEV=0 devices over DEV>0
                            accessPreference += 1;
                        } else {
                            isAdvanced = true;
                        }
                        x_phononId += ",DEV=" + deviceStr;
                        fallbackId += ",DEV=" + deviceStr;
                        if (handles.size() > 2 && handles.at(2).isValid()) {
                            x_phononId += ",SUBDEV=" + handles.at(2).toString();
                            fallbackId += ",SUBDEV=" + handles.at(2).toString();
                        }
                    }
                    deviceIds << x_phononId << fallbackId;
                }
            }
            break;

        case Solid::AudioInterface::OpenSoundSystem:
            if (audioIface->driverHandle().type() != QVariant::String) {
                valid = false;
            } else {
                const Solid::GenericInterface *genericIface =
                    hwDevice.as<Solid::GenericInterface>();
                Q_ASSERT(genericIface);
                cardNum = genericIface->property("oss.card").toInt();
                deviceNum = genericIface->property("oss.device").toInt();
                driver = PS::DeviceAccess::OssDriver;
                deviceIds << audioIface->driverHandle().toString();
            }
            break;
        }

        if (!valid || audioIface->soundcardType() == Solid::AudioInterface::Modem) {
            continue;
        }

        m_udisOfDevices.append(hwDevice.udi());

        const PS::DeviceAccess devAccess(deviceIds, accessPreference, driver, capture, playback);
        int initialPreference = 36 - deviceNum;

        QString uniqueIdPrefix = uniqueId(hwDevice, deviceNum);
        // "fix" cards that have the same identifiers, i.e. there's no way for the computer to tell
        // them apart.
        // We see that there's a problematic case if the same uniqueIdPrefix has been used for a
        // different cardNum before. In that case we need to append another number to the
        // uniqueIdPrefix. The first different cardNum gets a :i1, the second :i2, and so on.
        QList<int> &cardsForUniqueId = listOfCardNumsPerUniqueId[uniqueIdPrefix];
        if (cardsForUniqueId.isEmpty()) {
            cardsForUniqueId << cardNum;
        } else if (!cardsForUniqueId.contains(cardNum)) {
            cardsForUniqueId << cardNum;
            uniqueIdPrefix += QString(":i%1").arg(cardsForUniqueId.size() - 1);
        } else if (cardsForUniqueId.size() > 1) {
            const int listIndex = cardsForUniqueId.indexOf(cardNum);
            if (listIndex > 0) {
                uniqueIdPrefix += QString(":i%1").arg(listIndex);
            }
        }

        const PS::DeviceKey pkey = {
            uniqueIdPrefix + QLatin1String(":playback"), cardNum, deviceNum
        };
        const bool needNewPlaybackDevice = playback && !audioPlaybackDevices.contains(pkey);

        const PS::DeviceKey ckey = {
            uniqueIdPrefix + QLatin1String(":capture"), cardNum, deviceNum
        };
        const bool needNewCaptureDevice = capture && !audioCaptureDevices.contains(ckey);

        if (needNewPlaybackDevice || needNewCaptureDevice) {
            const QString &icon = hwDevice.icon();

            // Adjust the device preference according to the soudcard type
            switch (audioIface->soundcardType()) {
            case Solid::AudioInterface::InternalSoundcard:
                break;
            case Solid::AudioInterface::UsbSoundcard:
                initialPreference -= 10;
                break;
            case Solid::AudioInterface::FirewireSoundcard:
                initialPreference -= 15;
                break;
            case Solid::AudioInterface::Headset:
                initialPreference -= 10;
                break;
            case Solid::AudioInterface::Modem:
                initialPreference -= 1000;
                kWarning(601) << "Modem devices should never show up!";
                break;
            }

            if (needNewPlaybackDevice) {
                PS::DeviceInfo dev(PS::DeviceInfo::Audio, audioIface->name(), icon, pkey, initialPreference, isAdvanced);
                dev.addAccess(devAccess);
                audioPlaybackDevices.insert(pkey, dev);
            }

            if (needNewCaptureDevice) {
                PS::DeviceInfo dev(PS::DeviceInfo::Audio, audioIface->name(), icon, ckey, initialPreference, isAdvanced);
                dev.addAccess(devAccess);
                audioCaptureDevices.insert(ckey, dev);
            }
        }

        if (!needNewPlaybackDevice && playback) {
            PS::DeviceInfo &dev = audioPlaybackDevices[pkey];
            if (preferCardName) {
                dev.setPreferredName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }

        if (!needNewCaptureDevice && capture) {
            PS::DeviceInfo &dev = audioCaptureDevices[ckey];
            if (preferCardName) {
                dev.setPreferredName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }
    }

    m_audioOutputDevices = audioPlaybackDevices.values();
    m_audioCaptureDevices = audioCaptureDevices.values();

    if (haveAlsaDevices) {
        // go through the lists and check for devices that have only OSS and remove them since
        // they're very likely bogus (Solid tells us a device can do capture and playback, even
        // though it doesn't actually know that).
        removeOssOnlyDevices(&m_audioOutputDevices);
        removeOssOnlyDevices(&m_audioCaptureDevices);
    }

    /*
     * Process video devices
     */
    foreach (const Solid::Device &hwDevice, solidVideoDevices) {
        const Solid::Video *videoDevice = hwDevice.as<Solid::Video>();
        const Solid::Block *blockDevice = hwDevice.as<Solid::Block>();

        if (!videoDevice || !blockDevice)
            continue;

        if (videoDevice->supportedDrivers().isEmpty()) {
            continue;
        }

        kDebug(601) << "Solid video device:" << hwDevice.product() << hwDevice.description();
        foreach (const QString & driverName, videoDevice->supportedDrivers()) {
            kDebug(601) << "- driver" << driverName << ":" << videoDevice->driverHandle(driverName);
        }

        // Iterate through the supported drivers to create different access objects for each one
        foreach (const QString & driverName, videoDevice->supportedDrivers()) {
            deviceIds.clear();
            accessPreference = 0;
            driver = PS::DeviceAccess::InvalidDriver;
            isAdvanced = false;

            QVariant handle = videoDevice->driverHandle(driverName);

            if (handle.isValid()) {
                kDebug(601) << driverName << "valid handle, type" << handle.typeName();
            } else {
                kDebug(601) << driverName << "no driver handle";
            }

            if (hwDevice.udi().contains(QLatin1String("video4linux"))) {
                driver = PS::DeviceAccess::Video4LinuxDriver;
                deviceIds << blockDevice->device();
            }
            accessPreference += 20;

            if (handle.isValid())
                deviceIds << handle.toString();

            /*
             * TODO Check v4l docs or something to see if there's anything
             * else to do here
             */

            m_udisOfDevices.append(hwDevice.udi());

            PS::DeviceAccess devAccess(deviceIds, accessPreference, driver, true, false);
            devAccess.setPreferredDriverName(QString("%1 (%2)").arg(devAccess.driverName(), driverName));
            int initialPreference = 50;

            const PS::DeviceKey key = { uniqueId(hwDevice, -1), -1, -1 };
            const bool needNewDevice = !videoCaptureDevices.contains(key);

            if (needNewDevice) {
                const QString &icon = hwDevice.icon();

                // TODO Tweak initial preference using info from Solid

                // Create a new video capture device
                PS::DeviceInfo dev(PS::DeviceInfo::Video, hwDevice.product(), icon, key, initialPreference, isAdvanced);
                dev.addAccess(devAccess);
                videoCaptureDevices.insert(key, dev);
            } else {
                PS::DeviceInfo &dev = videoCaptureDevices[key];
                dev.addAccess(devAccess);
            }
        }
    }

    m_videoCaptureDevices = videoCaptureDevices.values();

    /* Now that we know about the hardware let's see what virtual devices we can find in
     * ~/.asoundrc and /etc/asound.conf
     */
    findVirtualDevices();

    QSet<QString> alreadyFoundCards;
    foreach (const PS::DeviceInfo &dev, m_audioOutputDevices) {
        alreadyFoundCards.insert(QLatin1String("AudioDevice_") + dev.key().uniqueId);
    }

    foreach (const PS::DeviceInfo &dev, m_audioCaptureDevices) {
        alreadyFoundCards.insert(QLatin1String("AudioDevice_") + dev.key().uniqueId);
    }

    foreach (const PS::DeviceInfo &dev, m_videoCaptureDevices) {
        alreadyFoundCards.insert(QLatin1String("VideoDevice_") + dev.key().uniqueId);
    }

    // now look in the config file for disconnected devices
    const QStringList &groupList = m_config->groupList();
    QStringList askToRemoveAudio;
    QStringList askToRemoveVideo;
    QList<int> askToRemoveAudioIndexes;
    QList<int> askToRemoveVideoIndexes;

    kDebug(601) << "groups:" << groupList;
    kDebug(601) << "already found devices:" << alreadyFoundCards;

    foreach (const QString &groupName, groupList) {
        if (alreadyFoundCards.contains(groupName)) {
            continue;
        }

        kDebug(601) << "group not found:" << groupName;

        const bool isAudio = groupName.startsWith(QLatin1String("AudioDevice_"));
        const bool isVideo = groupName.startsWith(QLatin1String("VideoDevice_"));
        const bool isPlayback = isAudio && groupName.endsWith(QLatin1String("playback"));
        const bool isCapture = isAudio && groupName.endsWith(QLatin1String("capture"));

        if (!isAudio && !isVideo) {
            continue;
        }

        if (isAudio && (!isPlayback && !isCapture)) {
            // this entry shouldn't be here
            m_config->deleteGroup(groupName);
        }

        const KConfigGroup cGroup(m_config, groupName);
        if (cGroup.readEntry("deleted", false)) {
            continue;
        }

        const QString &cardName = cGroup.readEntry("cardName", QString());
        const QString &iconName = cGroup.readEntry("iconName", QString());
        const bool hotpluggable = cGroup.readEntry("hotpluggable", true);
        const int initialPreference = cGroup.readEntry("initialPreference", 0);
        const int isAdvanced = cGroup.readEntry("isAdvanced", true);
        const int deviceNumber = cGroup.readEntry("deviceNumber", -1);
        const PS::DeviceKey key = { groupName.mid(12), -1, deviceNumber };
        const PS::DeviceInfo dev(PS::DeviceInfo::Audio, cardName, iconName, key, initialPreference, isAdvanced);

        if (!hotpluggable) {
            const QSettings phononSettings(QLatin1String("kde.org"), QLatin1String("libphonon"));
            if (isAdvanced && phononSettings.value(QLatin1String("General/HideAdvancedDevices"), true).toBool()) {
                dev.removeFromCache(m_config);
                continue;
            } else {
                if (isAudio) {
                    askToRemoveAudio << (isPlayback ? i18n("Output: %1", cardName) : i18n("Capture: %1", cardName));
                    askToRemoveAudioIndexes << cGroup.readEntry("index", 0);
                }

                if (isVideo) {
                    askToRemoveVideo << i18n("Video: %1", cardName);
                    askToRemoveVideoIndexes << cGroup.readEntry("index", 0);
                }
            }
        }

        if (isPlayback) {
            m_audioOutputDevices << dev;
        }

        if (isCapture) {
            m_audioCaptureDevices << dev;
        }

        if (isVideo) {
            m_videoCaptureDevices << dev;
        }

        alreadyFoundCards.insert(groupName);
    }

    if (!askToRemoveAudio.isEmpty()) {
        qSort(askToRemoveAudio);
        QMetaObject::invokeMethod(this, "askToRemoveDevices", Qt::QueuedConnection,
                Q_ARG(QStringList, askToRemoveAudio),
                Q_ARG(int, AudioOutputDeviceType | AudioCaptureDeviceType),
                Q_ARG(QList<int>, askToRemoveAudioIndexes));
    }

    if (!askToRemoveVideo.isEmpty()) {
        qSort(askToRemoveVideo);
        QMetaObject::invokeMethod(this, "askToRemoveDevices", Qt::QueuedConnection,
                Q_ARG(QStringList, askToRemoveVideo),
                Q_ARG(int, VideoCaptureDeviceType),
                Q_ARG(QList<int>, askToRemoveVideoIndexes));
    }

    renameDevices(&m_audioOutputDevices);
    renameDevices(&m_audioCaptureDevices);
    renameDevices(&m_videoCaptureDevices);

    qSort(m_audioOutputDevices);
    qSort(m_audioCaptureDevices);
    qSort(m_videoCaptureDevices);

    QMutableListIterator<PS::DeviceInfo> it(m_audioOutputDevices);
    while (it.hasNext()) {
        it.next().syncWithCache(m_config);
    }

    it = m_audioCaptureDevices;
    while (it.hasNext()) {
        it.next().syncWithCache(m_config);
    }

    it = m_videoCaptureDevices;
    while (it.hasNext()) {
        it.next().syncWithCache(m_config);
    }

    m_config->sync();

    kDebug(601) << "Audio Playback Devices:" << m_audioOutputDevices;
    kDebug(601) << "Audio Capture Devices:" << m_audioCaptureDevices;
    kDebug(601) << "Video Capture Devices:" << m_videoCaptureDevices;
}

QByteArray PhononServer::audioDevicesIndexes(int type)
{
    QByteArray *v;

    switch (type) {
    case AudioOutputDeviceType:
        v = &m_audioOutputDevicesIndexesCache;
        break;
    case AudioCaptureDeviceType:
        v = &m_audioCaptureDevicesIndexesCache;
        break;
    default:
        return QByteArray();
    }

    if (v->isEmpty()) {
        updateDevicesCache();
    }

    return *v;
}

QByteArray PhononServer::videoDevicesIndexes(int type)
{
    if (type != VideoCaptureDeviceType)
        return QByteArray();

    if (m_videoCaptureDevicesIndexesCache.isEmpty()) {
        updateDevicesCache();
    }

    return m_videoCaptureDevicesIndexesCache;
}

QByteArray PhononServer::audioDevicesProperties(int index)
{
    if (m_audioOutputDevicesIndexesCache.isEmpty() || m_audioCaptureDevicesIndexesCache.isEmpty()) {
        updateDevicesCache();
    }

    if (m_audioDevicesPropertiesCache.contains(index)) {
        return m_audioDevicesPropertiesCache.value(index);
    }

    return QByteArray();
}

QByteArray PhononServer::videoDevicesProperties(int index)
{
    if (m_videoCaptureDevicesIndexesCache.isEmpty()) {
        updateDevicesCache();
    }

    if (m_videoDevicesPropertiesCache.contains(index)) {
        return m_videoDevicesPropertiesCache.value(index);
    }

    return QByteArray();
}

bool PhononServer::isAudioDeviceRemovable(int index) const
{
    if (!m_audioDevicesPropertiesCache.contains(index)) {
        return false;
    }

    const QList<PS::DeviceInfo> &deviceList = m_audioOutputDevices + m_audioCaptureDevices;
    foreach (const PS::DeviceInfo &dev, deviceList) {
        if (dev.index() == index) {
            return !dev.isAvailable();
        }
    }

    return false;
}

bool PhononServer::isVideoDeviceRemovable(int index) const
{
    if (!m_videoDevicesPropertiesCache.contains(index)) {
        return false;
    }

    foreach (const PS::DeviceInfo &dev, m_videoCaptureDevices) {
        if (dev.index() == index) {
            return !dev.isAvailable();
        }
    }

    return false;
}

void PhononServer::removeAudioDevices(const QList<int> &indexes)
{
    const QList<PS::DeviceInfo> &deviceList = m_audioOutputDevices + m_audioCaptureDevices;
    foreach (int index, indexes) {
        foreach (const PS::DeviceInfo &dev, deviceList) {
            if (dev.index() == index) {
                if (!dev.isAvailable()) {
                    dev.removeFromCache(m_config);
                }
                break;
            }
        }
    }

    m_config->sync();
    m_updateDevicesTimer.start(50, this);
}

void PhononServer::removeVideoDevices(const QList< int >& indexes)
{
    foreach (int index, indexes) {
        foreach (const PS::DeviceInfo &dev, m_videoCaptureDevices) {
            if (dev.index() == index) {
                if (!dev.isAvailable()) {
                    dev.removeFromCache(m_config);
                }
                break;
            }
        }
    }

    m_config->sync();
    m_updateDevicesTimer.start(50, this);
}

static inline QByteArray nameForDriver(PS::DeviceAccess::DeviceDriverType d)
{
    switch (d) {
    case PS::DeviceAccess::AlsaDriver:
        return "alsa";
    case PS::DeviceAccess::OssDriver:
        return "oss";
    case PS::DeviceAccess::JackdDriver:
        return "jackd";
    case PS::DeviceAccess::Video4LinuxDriver:
        return "v4l2";
    case PS::DeviceAccess::InvalidDriver:
        break;
    }
    Q_ASSERT_X(false, "nameForDriver", "unknown driver");
    return "";
}

template<class T>
inline static QByteArray streamToByteArray(const T &data)
{
    QByteArray r;
    QDataStream stream(&r, QIODevice::WriteOnly);
    stream << data;
    return r;
}

inline static void insertGenericProperties(const PS::DeviceInfo &dev, QHash<QByteArray, QVariant> &p)
{
    p.insert("name", dev.name());
    p.insert("description", dev.description());
    p.insert("available", dev.isAvailable());
    p.insert("initialPreference", dev.initialPreference());
    p.insert("isAdvanced", dev.isAdvanced());
    p.insert("icon", dev.icon());
    p.insert("discovererIcon", "kde");
}

inline static void insertDALProperty(const PS::DeviceInfo &dev, QHash<QByteArray, QVariant> &p)
{
    DeviceAccessList deviceAccessList;
    foreach (const PS::DeviceAccess &access, dev.accessList()) {
        const QByteArray &driver = nameForDriver(access.driver());
        foreach (const QString &deviceId, access.deviceIds()) {
            deviceAccessList << DeviceAccess(driver, deviceId);
        }
    }

    p.insert("deviceAccessList", QVariant::fromValue(deviceAccessList));
}

void PhononServer::updateDevicesCache()
{
    QList<int> indexList;
    foreach (const PS::DeviceInfo &dev, m_audioOutputDevices) {
        QHash<QByteArray, QVariant> properties;
        insertGenericProperties(dev, properties);

        DeviceAccessList deviceAccessList;
        bool first = true;
        QStringList oldDeviceIds;
        PS::DeviceAccess::DeviceDriverType driverId = PS::DeviceAccess::InvalidDriver;
        foreach (const PS::DeviceAccess &access, dev.accessList()) {
            const QByteArray &driver = nameForDriver(access.driver());
            if (first) {
                driverId = access.driver();
                // Phonon 4.2 compatibility
                properties.insert("driver", driver);
                first = false;
            }
            foreach (const QString &deviceId, access.deviceIds()) {
                if (access.driver() == driverId) {
                    oldDeviceIds << deviceId;
                }
                deviceAccessList << DeviceAccess(driver, deviceId);
            }
        }

        properties.insert("deviceAccessList", QVariant::fromValue(deviceAccessList));

        // Phonon 4.2 compatibility
        properties.insert("deviceIds", oldDeviceIds);

        indexList << dev.index();
        m_audioDevicesPropertiesCache.insert(dev.index(), streamToByteArray(properties));
    }
    m_audioOutputDevicesIndexesCache = streamToByteArray(indexList);

    indexList.clear();
    foreach (const PS::DeviceInfo &dev, m_audioCaptureDevices) {
        QHash<QByteArray, QVariant> properties;
        insertGenericProperties(dev, properties);
        insertDALProperty(dev, properties);

        indexList << dev.index();
        m_audioDevicesPropertiesCache.insert(dev.index(), streamToByteArray(properties));
    }
    m_audioCaptureDevicesIndexesCache = streamToByteArray(indexList);

    indexList.clear();
    foreach (const PS::DeviceInfo &dev, m_videoCaptureDevices) {
        QHash<QByteArray, QVariant> properties;
        insertGenericProperties(dev, properties);
        insertDALProperty(dev, properties);

        indexList << dev.index();
        m_videoDevicesPropertiesCache.insert(dev.index(), streamToByteArray(properties));
    }
    m_videoCaptureDevicesIndexesCache = streamToByteArray(indexList);
}

void PhononServer::deviceAdded(const QString &udi)
{
    kDebug(601) << udi;
    m_updateDevicesTimer.start(50, this);
}

void PhononServer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_updateDevicesTimer.timerId()) {
        m_updateDevicesTimer.stop();
        m_audioOutputDevices.clear();
        m_audioCaptureDevices.clear();
        m_videoCaptureDevices.clear();
        m_udisOfDevices.clear();
        findDevices();
        m_audioOutputDevicesIndexesCache.clear();
        m_audioCaptureDevicesIndexesCache.clear();
        m_videoCaptureDevicesIndexesCache.clear();

        QDBusMessage signal = QDBusMessage::createSignal("/modules/phononserver", "org.kde.PhononServer", "devicesChanged");
        QDBusConnection::sessionBus().send(signal);
    }
}

void PhononServer::deviceRemoved(const QString &udi)
{
    kDebug(601) << udi;
    if (m_udisOfDevices.contains(udi)) {
        m_updateDevicesTimer.start(50, this);
    }
}

void PhononServer::askToRemoveDevices(const QStringList &devList, int type, const QList< int >& indexes)
{
    bool areAudio = type & (AudioOutputDeviceType | AudioCaptureDeviceType);
    bool areVideo = type & VideoCaptureDeviceType;

    if (!areAudio && !areVideo)
        return;

    const QString &dontEverAsk = QLatin1String("phonon_always_forget_devices");
    const QString &dontAskAgainName = QLatin1String("phonon_forget_devices_") +
        devList.join(QLatin1String("_"));

    // Please note that dontEverAsk overrides the device specific lists
    // i.e. if it is set we always return
    KMessageBox::ButtonCode result;
    if (!KMessageBox::shouldBeShownYesNo(dontEverAsk, result) ||
            !KMessageBox::shouldBeShownYesNo(dontAskAgainName, result)) {
        if (result == KMessageBox::Yes) {
            if (areAudio) {
                kDebug(601) << "removeAudioDevices" << indexes;
                removeAudioDevices(indexes);
            }

            if (areVideo) {
                kDebug(601) << "removeVideoDevices" << indexes;
                removeVideoDevices(indexes);
            }
        }
        return;
    }

    class MyDialog: public KDialog
    {
        public:
            MyDialog() : KDialog(0, Qt::Dialog) {}

        protected:
            virtual void slotButtonClicked(int button)
            {
                if (button == KDialog::User1) {
                    kDebug(601) << "start kcm_phonon";
                    QProcess::startDetached(QLatin1String("kcmshell5"), QStringList(QLatin1String("kcm_phonon")));
                    reject();
                } else {
                    KDialog::slotButtonClicked(button);
                }
            }
    } *dialog = new MyDialog;
    dialog->setPlainCaption(areAudio ? i18n("Removed Sound Devices") : i18n("Removed Video Devices"));
    dialog->setButtons(KDialog::Yes | KDialog::No | KDialog::User1);
    QIcon icon = QIcon::fromTheme(areAudio ? "audio-card" : "camera-web");
    dialog->setWindowIcon(icon);
    dialog->setModal(false);
    KGuiItem yes(KStandardGuiItem::yes());
    yes.setToolTip(areAudio ? i18n("Forget about the sound devices.") : i18n("Forget about the video devices"));
    dialog->setButtonGuiItem(KDialog::Yes, yes);
    dialog->setButtonGuiItem(KDialog::No, KStandardGuiItem::no());
    dialog->setButtonGuiItem(KDialog::User1, KGuiItem(i18nc("short string for a button, it opens "
                    "the Phonon page of System Settings", "Manage Devices"),
                QIcon::fromTheme("preferences-system"),
                i18n("Open the System Settings page for device configuration where you can "
                    "manually remove disconnected devices from the cache.")));
    dialog->setEscapeButton(KDialog::No);
    dialog->setDefaultButton(KDialog::User1);

    bool checkboxResult = false;
    QDialogButtonBox::StandardButton res = KMessageBox::createKMessageBox(dialog, new QDialogButtonBox(), icon,
            i18n("<html><p>KDE detected that one or more internal devices were removed.</p>"
                "<p><b>Do you want KDE to permanently forget about these devices?</b></p>"
                "<p>This is the list of devices KDE thinks can be removed:<ul><li>%1</li></ul></p></html>",
            devList.join(QLatin1String("</li><li>"))),
            QStringList(),
            i18n("Do not ask again for these devices"),
            &checkboxResult, KMessageBox::Notify);

    result = (res == QDialogButtonBox::Yes ? KMessageBox::Yes : KMessageBox::No);
    if (result == KMessageBox::Yes) {
        if (areAudio) {
            kDebug(601) << "removeAudioDevices" << indexes;
            removeAudioDevices(indexes);
        }

        if (areVideo) {
            kDebug(601) << "removeVideoDevices" << indexes;
            removeVideoDevices(indexes);
        }
    }

    if (checkboxResult) {
        KMessageBox::saveDontShowAgainYesNo(dontAskAgainName, result);
    }
}

#include "phononserver.moc"
