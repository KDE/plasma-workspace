/*  This file is part of the KDE project
    Copyright (C) 2007-2008 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "kdeplatformplugin.h"
#include "kiomediastream.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QtPlugin>
#include <QtCore/QCoreApplication>

#include <k4aboutdata.h>
#include <kdebug.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kicon.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <knotification.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kconfiggroup.h>

#include "devicelisting.h"

namespace Phonon
{

K_GLOBAL_STATIC_WITH_ARGS(KComponentData, mainComponentData, (QCoreApplication::applicationName().isEmpty() ? "Qt Application" : QCoreApplication::applicationName().toUtf8()))
K_GLOBAL_STATIC_WITH_ARGS(KComponentData, phononComponentData, ("phonon"))

static void ensureMainComponentData()
{
    if (!KGlobal::hasMainComponent()) {
        // a pure Qt application does not have a KComponentData object,
        // we'll give it one.
        *mainComponentData;
        qAddPostRoutine(mainComponentData.destroy);
        Q_ASSERT(KGlobal::hasMainComponent());
    }
}

static const KComponentData &componentData()
{
    ensureMainComponentData();
    return *phononComponentData;
}

KdePlatformPlugin::KdePlatformPlugin()
    : m_devList(0)
{
    ensureMainComponentData();
    KGlobal::locale()->insertCatalog(QLatin1String("phonon_kde"));
}

KdePlatformPlugin::~KdePlatformPlugin()
{
    delete m_devList;
}

AbstractMediaStream *KdePlatformPlugin::createMediaStream(const QUrl &url, QObject *parent)
{
    return new KioMediaStream(url, parent);
}

QIcon KdePlatformPlugin::icon(const QString &name) const
{
    return KIcon(name);
}

void KdePlatformPlugin::notification(const char *notificationName, const QString &text,
        const QStringList &actions, QObject *receiver,
        const char *actionSlot) const
{
    KNotification *notification = new KNotification(notificationName);
    notification->setComponentName(componentData().componentName());
    notification->setText(text);
    //notification->setPixmap(...);
    notification->addContext(QLatin1String("Application"), KGlobal::mainComponent().componentName());
    if (!actions.isEmpty() && receiver && actionSlot) {
        notification->setActions(actions);
        QObject::connect(notification, SIGNAL(activated(unsigned int)), receiver, actionSlot);
    }
    notification->sendEvent();
}

QString KdePlatformPlugin::applicationName() const
{
    ensureMainComponentData();
    const K4AboutData *ad = KGlobal::mainComponent().aboutData();
    if (ad) {
        const QString programName = ad->programName();
        if (programName.isEmpty()) {
            return KGlobal::mainComponent().componentName();
        }
        return programName;
    }
    return KGlobal::mainComponent().componentName();
}

QObject *KdePlatformPlugin::createBackend(KService::Ptr newService)
{
    QString errorReason;
    QObject *backend = newService->createInstance<QObject>(0, QVariantList(), &errorReason);
    if (!backend) {
        const QLatin1String suffix("/phonon_backend/");
        const QStringList libFilter(newService->library() + QLatin1String(".*"));
        foreach (const QString &libPathBase, QCoreApplication::libraryPaths()) {
            const QString libPath = libPathBase + suffix;
            const QDir dir(libPath);
            foreach (const QString &pluginName, dir.entryList(libFilter, QDir::Files)) {
                QPluginLoader pluginLoader(libPath + pluginName);
                backend = pluginLoader.instance();
                if (backend) {
                    break;
                }
            }
            if (backend) {
                break;
            }
        }
    }
    if (!backend) {
        kError(600) << "Can not create backend object from factory for " <<
            newService->name() << ", " << newService->library() << ":\n" << errorReason;

        return 0;

        // keep the translated text below for reuse later
        KMessageBox::error(0,
                i18n("<qt>Unable to use the <b>%1</b> Multimedia Backend:<br/>%2</qt>",
                    newService->name(), errorReason));
    }

    kDebug() << "using backend: " << newService->name();
    // Backends can have own l10n, try loading their catalog based on the library name.
     KGlobal::locale()->insertCatalog(newService->library());
    return backend;
}

QObject *KdePlatformPlugin::createBackend()
{
    // Within this process, display the warning about missing backends
    // only once.
    static bool has_shown = false;
    ensureMainComponentData();
    const KService::List offers = KServiceTypeTrader::self()->query("PhononBackend",
            "Type == 'Service' and [X-KDE-PhononBackendInfo-InterfaceVersion] == 1");
    if (offers.isEmpty()) {
        if (!has_shown) {
#if defined(HAVE_KDE4_MULTIMEDIA)
            KMessageBox::error(0, i18n("Unable to find a Multimedia Backend"));
#endif
            has_shown = true;
        }
        return 0;
    }
    // Flag the warning as not shown, since if the next time the
    // list of backends is suddenly empty again the user should be
    // told.
    has_shown = false;

    KService::List::const_iterator it = offers.begin();
    const KService::List::const_iterator end = offers.end();
    while (it != end) {
        QObject *backend = createBackend(*it);
        if (backend) {
            return backend;
        }
        ++it;
    }
    return 0;
}

QObject *KdePlatformPlugin::createBackend(const QString &library, const QString &version)
{
    ensureMainComponentData();
    QString additionalConstraints = QLatin1String(" and Library == '") + library + QLatin1Char('\'');
    if (!version.isEmpty()) {
        additionalConstraints += QLatin1String(" and [X-KDE-PhononBackendInfo-Version] == '")
            + version + QLatin1Char('\'');
    }
    const KService::List offers = KServiceTypeTrader::self()->query(QLatin1String("PhononBackend"),
            QString("Type == 'Service' and [X-KDE-PhononBackendInfo-InterfaceVersion] == 1%1")
            .arg(additionalConstraints));
    if (offers.isEmpty()) {
        KMessageBox::error(0, i18n("Unable to find the requested Multimedia Backend"));
        return 0;
    }

    KService::List::const_iterator it = offers.begin();
    const KService::List::const_iterator end = offers.end();
    while (it != end) {
        QObject *backend = createBackend(*it);
        if (backend) {
            return backend;
        }
        ++it;
    }
    return 0;
}

bool KdePlatformPlugin::isMimeTypeAvailable(const QString &mimeType) const
{
    ensureMainComponentData();
    const KService::List offers = KServiceTypeTrader::self()->query("PhononBackend",
            "Type == 'Service' and [X-KDE-PhononBackendInfo-InterfaceVersion] == 1");
    if (!offers.isEmpty()) {
        return offers.first()->hasMimeType(mimeType);
    }
    return false;
}

void KdePlatformPlugin::saveVolume(const QString &outputName, qreal volume)
{
    ensureMainComponentData();
    KConfigGroup config(KSharedConfig::openConfig(), "Phonon::AudioOutput");
    config.writeEntry(outputName + "_Volume", volume);
}

qreal KdePlatformPlugin::loadVolume(const QString &outputName) const
{
    ensureMainComponentData();
    KConfigGroup config(KSharedConfig::openConfig(), "Phonon::AudioOutput");
    return config.readEntry<qreal>(outputName + "_Volume", 1.0);
}

void KdePlatformPlugin::ensureDeviceListingObject() const
{
    if (!m_devList) {
        m_devList = new DeviceListing;
        connect(m_devList, SIGNAL(objectDescriptionChanged(ObjectDescriptionType)),
                SIGNAL(objectDescriptionChanged(ObjectDescriptionType)));
    }
}

QList<int> KdePlatformPlugin::objectDescriptionIndexes(ObjectDescriptionType type) const
{
    switch (type) {
    case AudioOutputDeviceType:
    case AudioCaptureDeviceType:
    case VideoCaptureDeviceType:
        ensureDeviceListingObject();
        return m_devList->objectDescriptionIndexes(type);
    default:
        return QList<int>();
    }
}

QHash<QByteArray, QVariant> KdePlatformPlugin::objectDescriptionProperties(ObjectDescriptionType type, int index) const
{
    switch (type) {
    case AudioOutputDeviceType:
    case AudioCaptureDeviceType:
    case VideoCaptureDeviceType:
        ensureDeviceListingObject();
        return m_devList->objectDescriptionProperties(type, index);
    default:
        return QHash<QByteArray, QVariant>();
    }
}

DeviceAccessList KdePlatformPlugin::deviceAccessListFor(const AudioOutputDevice &d) const
{
    return deviceAccessListFor(d.property("deviceAccessList"), d.property("driver"), d.property("deviceIds"));
}

DeviceAccessList KdePlatformPlugin::deviceAccessListFor(const AudioCaptureDevice &d) const
{
    return deviceAccessListFor(d.property("deviceAccessList"), d.property("driver"), d.property("deviceIds"));
}

DeviceAccessList KdePlatformPlugin::deviceAccessListFor(const VideoCaptureDevice &d) const
{
    return deviceAccessListFor(d.property("deviceAccessList"), d.property("driver"), d.property("deviceIds"));
}

DeviceAccessList KdePlatformPlugin::deviceAccessListFor(
    const QVariant &dalVariant,
    const QVariant &driverVariant,
    const QVariant &deviceIdsVariant) const
{
    if (dalVariant.isValid()) {
        return qvariant_cast<DeviceAccessList>(dalVariant);
    }

    DeviceAccessList ret;
    if (driverVariant.isValid()) {
        const QByteArray &driver = driverVariant.toByteArray();
        const QStringList &deviceIds = deviceIdsVariant.toStringList();
        foreach (const QString &deviceId, deviceIds) {
            ret << QPair<QByteArray, QString>(driver, deviceId);
        }
    }
    return ret;
}


} // namespace Phonon

#include "kdeplatformplugin.moc"
// vim: sw=4 sts=4 et tw=100
