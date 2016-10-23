/*
 *   Copyright (C) 2007 John Tapsell <tapsell@kde.org>
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

#ifndef SYSTEMMONITORENGINE_H
#define SYSTEMMONITORENGINE_H

#include <Plasma/DataEngine>

#include <ksgrd/SensorClient.h>

#include <QStringList>
#include <QVector>

class QTimer;

/**
 * This class evaluates the basic expressions given in the interface.
 */
class SystemMonitorEngine : public Plasma::DataEngine, public KSGRD::SensorClient
{
    Q_OBJECT

    public:
        /** Inherited from Plasma::DataEngine.  Returns a list of all the sensors that ksysguardd knows about. */
        QStringList sources() const override;
        SystemMonitorEngine( QObject* parent, const QVariantList& args );
        ~SystemMonitorEngine() override;

    protected:
        bool sourceRequestEvent(const QString &name) override;
        /** inherited from SensorClient */
        void answerReceived( int id, const QList<QByteArray>&answer ) override;
        void sensorLost( int ) override;
        bool updateSourceEvent(const QString &sensorName) override;

    protected Q_SLOTS:
        void updateSensors();
        void updateMonitorsList();

    private:
        QVector<QString> m_sensors;
        QTimer* m_timer;
        int m_waitingFor;
};

#endif

