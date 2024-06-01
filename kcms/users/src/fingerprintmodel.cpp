/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "fingerprintmodel.h"
#include "fprintdevice.h"

FingerprintModel::FingerprintModel(QObject *parent)
    : QObject(parent)
    , m_managerDbusInterface(new NetReactivatedFprintManagerInterface(QStringLiteral("net.reactivated.Fprint"),
                                                                      QStringLiteral("/net/reactivated/Fprint/Manager"),
                                                                      QDBusConnection::systemBus(),
                                                                      this))
{
    auto reply = m_managerDbusInterface->GetDefaultDevice();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        setCurrentError(reply.error().message());
        return;
    }

    QDBusObjectPath path = reply.value();
    m_device = new FprintDevice(path, this);

    connect(m_device, &FprintDevice::enrollCompleted, this, &FingerprintModel::handleEnrollCompleted);
    connect(m_device, &FprintDevice::enrollStagePassed, this, &FingerprintModel::handleEnrollStagePassed);
    connect(m_device, &FprintDevice::enrollRetryStage, this, &FingerprintModel::handleEnrollRetryStage);
    connect(m_device, &FprintDevice::enrollFailed, this, &FingerprintModel::handleEnrollFailed);
}

FingerprintModel::~FingerprintModel()
{
    if (m_device) { // just in case device is claimed
        m_device->stopEnrolling();
        m_device->release();
    }
}

FprintDevice::ScanType FingerprintModel::scanType()
{
    return !m_device ? FprintDevice::Press : m_device->scanType();
}

QString FingerprintModel::currentError()
{
    return m_currentError;
}

void FingerprintModel::setCurrentError(const QString &error)
{
    if (m_currentError != error) {
        m_currentError = error;
        Q_EMIT currentErrorChanged();
    }
}

QString FingerprintModel::enrollFeedback()
{
    return m_enrollFeedback;
}

void FingerprintModel::setEnrollFeedback(const QString &feedback)
{
    m_enrollFeedback = feedback;
    Q_EMIT enrollFeedbackChanged();
}

bool FingerprintModel::currentlyEnrolling()
{
    return m_currentlyEnrolling;
}

bool FingerprintModel::deviceFound()
{
    return m_device != nullptr;
}

double FingerprintModel::enrollProgress()
{
    if (!deviceFound()) {
        return 0;
    }
    return (m_device->numOfEnrollStages() == 0) ? 1 : ((double)m_enrollStage) / m_device->numOfEnrollStages();
}

void FingerprintModel::setEnrollStage(int stage)
{
    m_enrollStage = stage;
    Q_EMIT enrollProgressChanged();
}

FingerprintModel::DialogState FingerprintModel::dialogState()
{
    return m_dialogState;
}

void FingerprintModel::setDialogState(DialogState dialogState)
{
    m_dialogState = dialogState;
    Q_EMIT dialogStateChanged();
}

void FingerprintModel::switchUser(const QString &username)
{
    m_username = username;

    if (deviceFound()) {
        stopEnrolling(); // stop enrolling if ongoing
        m_device->release(); // release from old user

        Q_EMIT enrolledFingerprintsChanged();
    }
}

bool FingerprintModel::claimDevice()
{
    if (!deviceFound()) {
        return false;
    }

    QDBusError error = m_device->claim(m_username);
    if (error.isValid() && error.name() != u"net.reactivated.Fprint.Error.AlreadyInUse") {
        qDebug() << "error claiming:" << error.message();
        setCurrentError(error.message());
        return false;
    }
    return true;
}

void FingerprintModel::startEnrolling(const QString &finger)
{
    if (!deviceFound()) {
        setCurrentError(i18n("No fingerprint device found."));
        setDialogState(DialogState::FingerprintList);
        return;
    }

    setEnrollStage(0);
    setEnrollFeedback({});

    // claim device for user
    if (!claimDevice()) {
        setDialogState(DialogState::FingerprintList);
        return;
    }

    QDBusError error = m_device->startEnrolling(finger);
    if (error.isValid()) {
        qDebug() << "error start enrolling:" << error.message();
        setCurrentError(error.message());
        m_device->release();
        setDialogState(DialogState::FingerprintList);
        return;
    }

    m_currentlyEnrolling = true;
    Q_EMIT currentlyEnrollingChanged();

    setDialogState(DialogState::Enrolling);
}

void FingerprintModel::stopEnrolling()
{
    setDialogState(DialogState::FingerprintList);
    if (m_currentlyEnrolling) {
        m_currentlyEnrolling = false;
        Q_EMIT currentlyEnrollingChanged();

        QDBusError error = m_device->stopEnrolling();
        if (error.isValid()) {
            qDebug() << "error stop enrolling:" << error.message();
            setCurrentError(error.message());
            return;
        }
        m_device->release();
    }
}

void FingerprintModel::deleteFingerprint(const QString &finger)
{
    // claim for user
    if (!claimDevice()) {
        return;
    }

    QDBusError error = m_device->deleteEnrolledFinger(finger);
    if (error.isValid()) {
        qDebug() << "error deleting fingerprint:" << error.message();
        setCurrentError(error.message());
    }

    // release from user
    error = m_device->release();
    if (error.isValid()) {
        qDebug() << "error releasing:" << error.message();
        setCurrentError(error.message());
    }

    Q_EMIT enrolledFingerprintsChanged();
}

QStringList FingerprintModel::enrolledFingerprintsRaw()
{
    if (deviceFound()) {
        QDBusPendingReply<QStringList> reply = m_device->listEnrolledFingers(m_username);
        reply.waitForFinished();
        if (reply.isError()) {
            // ignore net.reactivated.Fprint.Error.NoEnrolledPrints, as it shows up when there are no fingerprints
            if (reply.error().name() != u"net.reactivated.Fprint.Error.NoEnrolledPrints") {
                qDebug() << "error listing enrolled fingers:" << reply.error().message();
                setCurrentError(reply.error().message());
            }
            return QStringList();
        }
        return reply.value();
    } else {
        setCurrentError(i18n("No fingerprint device found."));
        setDialogState(DialogState::FingerprintList);
        return QStringList();
    }
}

QVariantList FingerprintModel::enrolledFingerprints()
{
    // convert fingers list to qlist of Finger objects
    QVariantList fingers;
    for (QString &finger : enrolledFingerprintsRaw()) {
        for (const Finger &storedFinger : FINGERS) {
            if (storedFinger.internalName() == finger) {
                fingers.append(QVariant::fromValue(storedFinger));
                break;
            }
        }
    }
    return fingers;
}

QVariantList FingerprintModel::availableFingersToEnroll()
{
    QVariantList list;
    QStringList enrolled = enrolledFingerprintsRaw();

    // add fingerprints to list that are not in the enrolled list
    for (const Finger &finger : FINGERS) {
        if (!enrolledFingerprintsRaw().contains(finger.internalName())) {
            list.append(QVariant::fromValue(finger));
        }
    }
    return list;
}

QVariantList FingerprintModel::unavailableFingersToEnroll()
{
    QVariantList list;
    QStringList enrolled = enrolledFingerprintsRaw();

    // add fingerprints to list that are in the enrolled list
    for (const Finger &finger : FINGERS) {
        if (enrolledFingerprintsRaw().contains(finger.internalName())) {
            list.append(QVariant::fromValue(finger));
        }
    }
    return list;
}

void FingerprintModel::handleEnrollCompleted()
{
    setEnrollStage(m_device->numOfEnrollStages());
    setEnrollFeedback({});
    Q_EMIT enrolledFingerprintsChanged();
    Q_EMIT scanComplete();

    // stopEnrolling not called, as it is triggered only when the "complete" button is pressed
    // (only change dialog state change after button is pressed)
    setDialogState(DialogState::EnrollComplete);
}

void FingerprintModel::handleEnrollStagePassed()
{
    setEnrollStage(m_enrollStage + 1);
    setEnrollFeedback({});
    Q_EMIT scanSuccess();
    qDebug() << "fingerprint enroll stage pass:" << enrollProgress();
}

void FingerprintModel::handleEnrollRetryStage(const QString &feedback)
{
    Q_EMIT scanFailure();
    if (feedback == u"enroll-retry-scan") {
        setEnrollFeedback(i18n("Retry scanning your finger."));
    } else if (feedback == u"enroll-swipe-too-short") {
        setEnrollFeedback(i18n("Swipe too short. Try again."));
    } else if (feedback == u"enroll-finger-not-centered") {
        setEnrollFeedback(i18n("Finger not centered on the reader. Try again."));
    } else if (feedback == u"enroll-remove-and-retry") {
        setEnrollFeedback(i18n("Remove your finger from the reader, and try again."));
    }
    qDebug() << "fingerprint enroll stage fail:" << feedback;
}

void FingerprintModel::handleEnrollFailed(const QString &error)
{
    if (error == u"enroll-failed") {
        setCurrentError(i18n("Fingerprint enrollment has failed."));
        stopEnrolling();
    } else if (error == u"enroll-data-full") {
        setCurrentError(i18n("There is no space left for this device, delete other fingerprints to continue."));
        stopEnrolling();
    } else if (error == u"enroll-disconnected") {
        setCurrentError(i18n("The device was disconnected."));
        m_currentlyEnrolling = false;
        Q_EMIT currentlyEnrollingChanged();
        setDialogState(DialogState::FingerprintList);
    } else if (error == u"enroll-unknown-error") {
        setCurrentError(i18n("An unknown error has occurred."));
        stopEnrolling();
    }
}

Finger::Finger(const QString &internalName, const QString &friendlyName)
    : m_internalName(internalName)
    , m_friendlyName(friendlyName)
{
}

QString Finger::friendlyName() const
{
    return m_friendlyName;
}

QString Finger::internalName() const
{
    return m_internalName;
}
