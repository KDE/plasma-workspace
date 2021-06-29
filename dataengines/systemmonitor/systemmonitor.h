/*
    SPDX-FileCopyrightText: 2007 John Tapsell <tapsell@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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
    SystemMonitorEngine(QObject *parent, const QVariantList &args);
    ~SystemMonitorEngine() override;

protected:
    bool sourceRequestEvent(const QString &name) override;
    /** inherited from SensorClient */
    void answerReceived(int id, const QList<QByteArray> &answer) override;
    void sensorLost(int) override;
    bool updateSourceEvent(const QString &sensorName) override;

protected Q_SLOTS:
    void updateSensors();
    void updateMonitorsList();

private:
    QVector<QString> m_sensors;
    QTimer *m_timer;
    int m_waitingFor;
};
