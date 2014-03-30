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

#include "systemmonitor.h"

#include <QTimer>
#include <QProcess>

#include <QDebug>
#include <KLocale>

#include <Plasma/DataContainer>

#include <ksgrd/SensorManager.h>

SystemMonitorEngine::SystemMonitorEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent)
{
    Q_UNUSED(args)

    KSGRD::SensorMgr = new KSGRD::SensorManager(this);
    KSGRD::SensorMgr->engage("localhost", "", "ksysguardd");

    m_waitingFor= 0;
    connect(KSGRD::SensorMgr, SIGNAL(update()), this, SLOT(updateMonitorsList()));
    updateMonitorsList();
}

SystemMonitorEngine::~SystemMonitorEngine()
{
}

void SystemMonitorEngine::updateMonitorsList()
{
    KSGRD::SensorMgr->sendRequest("localhost", "monitors", (KSGRD::SensorClient*)this, -1);
}

QStringList SystemMonitorEngine::sources() const
{
    return m_sensors;
}

bool SystemMonitorEngine::sourceRequestEvent(const QString &name)
{
    // NB: do not follow this example in your own data engines!
    // This is kept for backwards compatilibility.
    // Visualizations should instead listen to sourceAdded()
    if (m_sensors.isEmpty()) {
        // we don't have our first data yet, so let's trust the requester, at least fo rnow
        // when we get our list of sensors later, then we'll know for sure and remove
        // this source if they were wrong
        setData(name, DataEngine::Data());
        return true;
    }

    return false;
}

bool SystemMonitorEngine::updateSourceEvent(const QString &sensorName)
{
    const int index = m_sensors.indexOf(sensorName);

    if (index != -1) {
        KSGRD::SensorMgr->sendRequest("localhost", sensorName, (KSGRD::SensorClient*)this, index);
        KSGRD::SensorMgr->sendRequest("localhost", QString("%1?").arg(sensorName), (KSGRD::SensorClient*)this, -(index + 2));
    }

    return false;
}

void SystemMonitorEngine::updateSensors()
{
    DataEngine::SourceDict sources = containerDict();
    DataEngine::SourceDict::iterator it = sources.begin();

    m_waitingFor = 0;

    while (it != sources.end()) {
        m_waitingFor++;
        QString sensorName = it.key();
        KSGRD::SensorMgr->sendRequest( "localhost", sensorName, (KSGRD::SensorClient*)this, -1);
        ++it;
    }
}

void SystemMonitorEngine::answerReceived(int id, const QList<QByteArray> &answer)
{
    if (id < -1) {
        if (answer.isEmpty() || m_sensors.count() <= (-id - 2)) {
            qDebug() << "sensor info answer was empty, (" << answer.isEmpty() << ") or sensors does not exist to us ("
                     << (m_sensors.count() < (-id - 2)) << ") for index" << (-id - 2);
            return;
        }

        DataEngine::SourceDict sources = containerDict();
        DataEngine::SourceDict::const_iterator it = sources.constFind(m_sensors.value(-id - 2));

        const QStringList newSensorInfo = QString::fromUtf8(answer[0]).split('\t');

        if (newSensorInfo.count() < 4) {
            qDebug() << "bad sensor info, only" << newSensorInfo.count()
                     << "entries, and we were expecting 4. Answer was " << answer;
            if(it != sources.constEnd())
                qDebug() << "value =" << it.value()->data()["value"] << "type=" << it.value()->data()["type"];
            return;
        }

        const QString sensorName = newSensorInfo[0];
        const QString min = newSensorInfo[1];
        const QString max = newSensorInfo[2];
        const QString unit = newSensorInfo[3];

        if (it != sources.constEnd()) {
            it.value()->setData("name", sensorName);
            it.value()->setData("min", min);
            it.value()->setData("max", max);
            it.value()->setData("units", unit);
        }

        return;
    }

    if (id == -1) {
        QSet<QString> sensors;
        m_sensors.clear();
        int count = 0;

        foreach (const QByteArray &sens, answer) {
            const QStringList newSensorInfo = QString::fromUtf8(sens).split('\t');
            if (newSensorInfo.count() < 2) {
                continue;
            }
            if(newSensorInfo.at(1) == "logfile")
                continue; // logfile data type not currently supported

            const QString newSensor = newSensorInfo[0];
            sensors.insert(newSensor);
            m_sensors.append(newSensor);
            {
                // HACK: for backwards compability
                // in case this source was created in sourceRequestEvent, stop it being
                // automagically removed when disconnected from
                Plasma::DataContainer *s = containerForSource( newSensor );
                if ( s ) {
                    disconnect( s, SIGNAL(becameUnused(QString)), this, SLOT(removeSource(QString)) );
                }
            }
            DataEngine::Data d;
            d.insert("value", QVariant());
            d.insert("type", newSensorInfo[1]);
            setData(newSensor, d);
            KSGRD::SensorMgr->sendRequest( "localhost", QString("%1?").arg(newSensor), (KSGRD::SensorClient*)this, -(count + 2));
            ++count;
        }

        QHash<QString, Plasma::DataContainer*> sourceDict = containerDict();
        QHashIterator<QString, Plasma::DataContainer*> it(sourceDict);
        while (it.hasNext()) {
            it.next();
            if (!sensors.contains(it.key())) {
                removeSource(it.key());
            }
        }

        return;
    }

    m_waitingFor--;
    QString reply;
    if (!answer.isEmpty()) {
        reply = QString::fromUtf8(answer[0]);
    }

    DataEngine::SourceDict sources = containerDict();
    DataEngine::SourceDict::const_iterator it = sources.constFind(m_sensors.value(id));
    if (it != sources.constEnd()) {
        it.value()->setData("value", reply);
    }

}

void SystemMonitorEngine::sensorLost( int )
{
    m_waitingFor--;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(systemmonitor, SystemMonitorEngine, "plasma-dataengine-systemmonitor.json")

#include "systemmonitor.moc"

