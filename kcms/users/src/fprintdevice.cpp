/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "fprintdevice.h"

FprintDevice::FprintDevice(QDBusObjectPath path, QObject *parent)
    : QObject(parent)
    , m_devicePath(path.path())
    , m_fprintInterface(new NetReactivatedFprintDeviceInterface(QStringLiteral("net.reactivated.Fprint"), path.path(), QDBusConnection::systemBus(), this))
    , m_freedesktopInterface(
          new QDBusInterface(QStringLiteral("net.reactivated.Fprint"), path.path(), "org.freedesktop.DBus.Properties", QDBusConnection::systemBus(), this))
{
    connect(m_fprintInterface, &NetReactivatedFprintDeviceInterface::EnrollStatus, this, &FprintDevice::enrollStatus);
}

QDBusPendingReply<QStringList> FprintDevice::listEnrolledFingers(const QString &username)
{
    return m_fprintInterface->ListEnrolledFingers(username);
}

QDBusError FprintDevice::claim(const QString &username)
{
    auto reply = m_fprintInterface->Claim(username);
    reply.waitForFinished();
    return reply.error();
}

QDBusError FprintDevice::release()
{
    auto reply = m_fprintInterface->Release();
    reply.waitForFinished();
    return reply.error();
}

QDBusError FprintDevice::deleteEnrolledFingers()
{
    auto reply = m_fprintInterface->DeleteEnrolledFingers2();
    reply.waitForFinished();
    return reply.error();
}

QDBusError FprintDevice::deleteEnrolledFinger(QString &finger)
{
    auto reply = m_fprintInterface->DeleteEnrolledFinger(finger);
    reply.waitForFinished();
    return reply.error();
}

QDBusError FprintDevice::startEnrolling(const QString &finger)
{
    auto reply = m_fprintInterface->EnrollStart(finger);
    reply.waitForFinished();
    return reply.error();
}

QDBusError FprintDevice::stopEnrolling()
{
    auto reply = m_fprintInterface->EnrollStop();
    reply.waitForFinished();
    return reply.error();
}

void FprintDevice::enrollStatus(QString result, bool done)
{
    Q_UNUSED(done)

    if (result == "enroll-completed") {
        Q_EMIT enrollCompleted();
    } else if (result == "enroll-failed" || result == "enroll-data-full" || result == "enroll-disconnected" || result == "enroll-unknown-error") {
        Q_EMIT enrollFailed(result);
    } else if (result == "enroll-stage-passed") {
        Q_EMIT enrollStagePassed();
    } else if (result == "enroll-retry-scan" || result == "enroll-swipe-too-short" || result == "enroll-finger-not-centered"
               || result == "enroll-remove-and-retry") {
        Q_EMIT enrollRetryStage(result);
    }
}

int FprintDevice::numOfEnrollStages()
{
    QDBusReply<QDBusVariant> reply = m_freedesktopInterface->call("Get", "net.reactivated.Fprint.Device", "num-enroll-stages");
    if (!reply.isValid()) {
        qDebug() << "error fetching num-enroll-stages:" << reply.error();
        return 0;
    }
    return reply.value().variant().toInt();
}

FprintDevice::ScanType FprintDevice::scanType()
{
    QDBusReply<QDBusVariant> reply = m_freedesktopInterface->call("Get", "net.reactivated.Fprint.Device", "scan-type");
    if (!reply.isValid()) {
        qDebug() << "error fetching scan-type:" << reply.error();
        return FprintDevice::Press;
    }

    const QString type = reply.value().variant().toString();

    if (type == QLatin1String("press")) {
        return FprintDevice::Press;
    }

    if (type == QLatin1String("swipe")) {
        return FprintDevice::Swipe;
    }

    qWarning() << "Unknown fprint scan-type:" << type;

    return FprintDevice::Press;
}

bool FprintDevice::fingerPresent()
{
    QDBusReply<QDBusVariant> reply = m_freedesktopInterface->call("Get", "net.reactivated.Fprint.Device", "finger-present");
    if (!reply.isValid()) {
        qDebug() << "error fetching finger-present:" << reply.error();
        return "";
    }
    return reply.value().variant().toBool();
}

bool FprintDevice::fingerNeeded()
{
    QDBusReply<QDBusVariant> reply = m_freedesktopInterface->call("Get", "net.reactivated.Fprint.Device", "finger-needed");
    if (!reply.isValid()) {
        qDebug() << "error fetching finger-needed:" << reply.error();
        return "";
    }
    return reply.value().variant().toBool();
}
