/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "fprint_device_interface.h"
#include "fprint_manager_interface.h"

class FprintDevice : public QObject
{
    Q_OBJECT

public:
    enum ScanType {
        Press,
        Swipe,
    };
    Q_ENUM(ScanType);

    explicit FprintDevice(QDBusObjectPath path, QObject *parent = nullptr);

    QDBusPendingReply<QStringList> listEnrolledFingers(const QString &username);

    QDBusError claim(const QString &username);
    QDBusError release();

    QDBusError deleteEnrolledFingers();
    QDBusError deleteEnrolledFinger(QString &finger);
    QDBusError startEnrolling(const QString &finger);
    QDBusError stopEnrolling();

    int numOfEnrollStages();
    ScanType scanType();
    bool fingerPresent();
    bool fingerNeeded();

public Q_SLOTS:
    void enrollStatus(QString result, bool done);

Q_SIGNALS:
    void enrollCompleted();
    void enrollStagePassed();
    void enrollRetryStage(QString feedback);
    void enrollFailed(QString error);

private:
    QString m_devicePath;
    QString m_username;
    NetReactivatedFprintDeviceInterface *m_fprintInterface;
    QDBusInterface *m_freedesktopInterface;
};
