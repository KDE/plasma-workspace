/*  This file is part of the KDE project
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>

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

#ifndef PHONONSERVER_H
#define PHONONSERVER_H

#include "deviceinfo.h"

#include <kdedmodule.h>
#include <ksharedconfig.h>
#include <phonon/objectdescription.h>
#include <QtCore/QBasicTimer>
#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtDBus/QDBusVariant>

class PhononServer : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.PhononServer")
    public:
        PhononServer(QObject *parent, const QList<QVariant> &args);
        ~PhononServer();

    public slots:
        Q_SCRIPTABLE QByteArray audioDevicesIndexes(int type);
        Q_SCRIPTABLE QByteArray videoDevicesIndexes(int type);
        Q_SCRIPTABLE QByteArray audioDevicesProperties(int index);
        Q_SCRIPTABLE QByteArray videoDevicesProperties(int index);
        Q_SCRIPTABLE bool isAudioDeviceRemovable(int index) const;
        Q_SCRIPTABLE bool isVideoDeviceRemovable(int index) const;
        Q_SCRIPTABLE void removeAudioDevices(const QList<int> &indexes);
        Q_SCRIPTABLE void removeVideoDevices(const QList<int> &indexes);

    protected:
        void timerEvent(QTimerEvent *e);

    private slots:
        void deviceAdded(const QString &udi);
        void deviceRemoved(const QString &udi);
        // TODO add callbacks for Jack changes and whatever else, if somehow possible (Pulse handled by libphonon)

        void alsaConfigChanged();

        void askToRemoveDevices(const QStringList &devList, int type, const QList<int> &indexes);

    private:
        void findDevices();
        void findVirtualDevices();
        void updateDevicesCache();

        KSharedConfigPtr m_config;
        QBasicTimer m_updateDevicesTimer;

        // cache
        QByteArray m_audioOutputDevicesIndexesCache;
        QByteArray m_audioCaptureDevicesIndexesCache;
        QByteArray m_videoCaptureDevicesIndexesCache;
        QHash<int, QByteArray> m_audioDevicesPropertiesCache;
        QHash<int, QByteArray> m_videoDevicesPropertiesCache;

        // devices ordered by preference
        QList<PS::DeviceInfo> m_audioOutputDevices;
        QList<PS::DeviceInfo> m_audioCaptureDevices;
        QList<PS::DeviceInfo> m_videoCaptureDevices;

        QStringList m_udisOfDevices;
};

#endif // PHONONSERVER_H
