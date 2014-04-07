/*  This file is part of the KDE project
    Copyright (C) 2006-2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) version 3.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "devicelisting.h"

#include <QtCore/QFile>
#include <QtDBus/QDBusReply>
#include <QtCore/QMutableHashIterator>
#include <QtCore/QTimerEvent>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <klocale.h>
#include <ksharedconfig.h>

#include <../config-alsa.h>
#ifdef HAVE_ALSA_ASOUNDLIB_H
#include <alsa/asoundlib.h>
#endif // HAVE_ALSA_ASOUNDLIB_H

static void installAlsaPhononDeviceHandle()
{
#ifdef HAVE_LIBASOUND2
    // after recreating the global configuration we can go and install custom configuration
    snd_config_update_free_global();
    snd_config_update();
    Q_ASSERT(snd_config);

    // x-phonon: device
    QFile phononDefinition(":/phonon/phonondevice.alsa");
    phononDefinition.open(QIODevice::ReadOnly);
    const QByteArray &phononDefinitionData = phononDefinition.readAll();

    snd_input_t *sndInput = 0;
    if (0 == snd_input_buffer_open(&sndInput, phononDefinitionData.constData(), phononDefinitionData.size())) {
        Q_ASSERT(sndInput);
        snd_config_load(snd_config, sndInput);
        snd_input_close(sndInput);
    }

#if 0
    // phonon_softvol: device
    QFile softvolDefinition(":/phonon/softvol.alsa");
    softvolDefinition.open(QIODevice::ReadOnly);
    const QByteArray softvolDefinitionData = softvolDefinition.readAll();

    sndInput = 0;
    if (0 == snd_input_buffer_open(&sndInput, softvolDefinitionData.constData(), softvolDefinitionData.size())) {
        Q_ASSERT(sndInput);
        snd_config_load(snd_config, sndInput);
        snd_input_close(sndInput);
    }
#endif
#endif // HAVE_LIBASOUND2
}

namespace Phonon
{

QList<int> DeviceListing::objectDescriptionIndexes(ObjectDescriptionType type)
{
    QList<int> r;
    QDBusReply<QByteArray> reply;

    if (type == AudioOutputDeviceType || type == AudioCaptureDeviceType) {
        reply = m_phononServer.call(QLatin1String("audioDevicesIndexes"), static_cast<int>(type));
        if (!reply.isValid()) {
            kError(600) << reply.error();
            return r;
        }
    } else
    if (type == VideoCaptureDeviceType) {
        reply = m_phononServer.call(QLatin1String("videoDevicesIndexes"), static_cast<int>(type));
        if (!reply.isValid()) {
            kError(600) << reply.error();
            return r;
        }
    } else
        return r;

    QDataStream stream(reply.value());
    stream >> r;
    return r;
}

QHash<QByteArray, QVariant> DeviceListing::objectDescriptionProperties(ObjectDescriptionType type, int index)
{
    QHash<QByteArray, QVariant> r;
    QDBusReply<QByteArray> reply;

    if (type == AudioOutputDeviceType || type == AudioCaptureDeviceType) {
        reply = m_phononServer.call(QLatin1String("audioDevicesProperties"), index);
        if (!reply.isValid()) {
            kError(600) << reply.error();
            return r;
        }
    } else
    if (type == VideoCaptureDeviceType) {
        reply = m_phononServer.call(QLatin1String("videoDevicesProperties"), index);
        if (!reply.isValid()) {
            kError(600) << reply.error();
            return r;
        }
    } else
        return r;

    QDataStream stream(reply.value());
    stream >> r;
    return r;
}

DeviceListing::DeviceListing()
    : m_phononServer(
            QLatin1String("org.kde.kded5"),
            QLatin1String("/modules/phononserver"),
            QLatin1String("org.kde.PhononServer"))
{
    KSharedConfigPtr config;
    config = KSharedConfig::openConfig("phonon_platform_kde");
    installAlsaPhononDeviceHandle();

    QDBusConnection::sessionBus().connect(QLatin1String("org.kde.kded5"), QLatin1String("/modules/phononserver"), QLatin1String("org.kde.PhononServer"),
            QLatin1String("devicesChanged"), QString(), this, SLOT(devicesChanged()));
}

DeviceListing::~DeviceListing()
{
}

void DeviceListing::devicesChanged()
{
    kDebug(600);
    m_signalTimer.start(0, this);
}

void DeviceListing::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_signalTimer.timerId()) {
        m_signalTimer.stop();
        kDebug(600) << "emitting objectDescriptionChanged for all devices";
        emit objectDescriptionChanged(AudioOutputDeviceType);
        emit objectDescriptionChanged(AudioCaptureDeviceType);
        emit objectDescriptionChanged(VideoCaptureDeviceType);
    }
}

} // namespace Phonon
