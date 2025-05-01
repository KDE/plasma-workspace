/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "fprint_device_interface.h"
#include "properties_interface.h"
#include <qqmlintegration.h>

class FprintDevice : public QObject
{
    Q_OBJECT
    QML_ELEMENT

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

    QDBusError deleteEnrolledFinger(const QString &finger);
    QDBusError startEnrolling(const QString &finger);
    QDBusError stopEnrolling();

    int numOfEnrollStages();
    ScanType scanType();
    bool fingerPresent();
    bool fingerNeeded();

public Q_SLOTS:
    void enrollStatus(const QString &result, bool done);

Q_SIGNALS:
    void enrollCompleted();
    void enrollStagePassed();
    void enrollRetryStage(const QString &feedback);
    void enrollFailed(const QString &error);

private:
    QString m_devicePath;
    QString m_username;
    NetReactivatedFprintDeviceInterface *m_fprintInterface;
    OrgFreedesktopDBusPropertiesInterface *m_properiesInterface;
};
