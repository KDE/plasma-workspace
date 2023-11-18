/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>

#include <KAuthorized>

// kde-workspace/libs
#include <sessionmanagement.h>

#include <krunner_interface.h>

#include "powermanagementjob.h"

PowerManagementJob::PowerManagementJob(const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent)
    : ServiceJob(parent->objectName(), operation, parameters, parent)
    , m_session(new SessionManagement(this))
{
}

PowerManagementJob::~PowerManagementJob()
{
}

static void callWhenFinished(const QDBusPendingCall &pending, std::function<void(bool)> func, QObject *parent)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, parent);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, parent, [func](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        func(!watcher->isError());
    });
}

void PowerManagementJob::start()
{
    const QString operation = operationName();
    // qDebug() << "starting operation  ... " << operation;

    if (operation == QLatin1String("lockScreen")) {
        if (m_session->canLock()) {
            m_session->lock();
            setResult(true);
            return;
        }
        qDebug() << "operation denied " << operation;
        setResult(false);
        return;
    } else if (operation == QLatin1String("suspend") || operation == QLatin1String("suspendToRam")) {
        if (m_session->canSuspend()) {
            m_session->suspend();
            setResult(true);
        } else {
            setResult(false);
        }
        return;
    } else if (operation == QLatin1String("suspendToDisk")) {
        if (m_session->canHibernate()) {
            m_session->hibernate();
            setResult(true);
        } else {
            setResult(false);
        }
        return;
    } else if (operation == QLatin1String("suspendHybrid")) {
        if (m_session->canHybridSuspend()) {
            m_session->hybridSuspend();
            setResult(true);
        } else {
            setResult(false);
        }
        return;
    } else if (operation == QLatin1String("requestShutDown")) {
        if (m_session->canShutdown()) {
            m_session->requestShutdown();
            setResult(true);
        } else {
            setResult(false);
        }
        return;
    } else if (operation == QLatin1String("switchUser")) {
        if (m_session->canSwitchUser()) {
            m_session->switchUser();
            setResult(true);
        }
        setResult(false);
        return;
    } else if (operation == QLatin1String("beginSuppressingSleep")) {
        if (m_sleepInhibitionCookie != -1) { // an inhibition request is already active; don't trigger another one
            setResult(true);
            return;
        }
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                          QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                          QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                          QStringLiteral("Inhibit"));
        msg << QCoreApplication::applicationName() << parameters().value(QStringLiteral("reason")).toString();
        QDBusReply<uint> reply = QDBusConnection::sessionBus().call(msg);
        m_sleepInhibitionCookie = reply.isValid() ? reply.value() : -1;
        setResult(reply.isValid());
        return;
    } else if (operation == QLatin1String("stopSuppressingSleep")) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                          QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                          QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                          QStringLiteral("UnInhibit"));
        msg << m_sleepInhibitionCookie;
        QDBusReply<void> reply = QDBusConnection::sessionBus().call(msg);
        m_sleepInhibitionCookie = reply.isValid() ? -1 : m_sleepInhibitionCookie; // reset cookie if the stop request was successful
        setResult(reply.isValid());
        return;
    } else if (operation == QLatin1String("beginSuppressingScreenPowerManagement")) {
        if (m_lockInhibitionCookie != -1) { // an inhibition request is already active; don't trigger another one
            setResult(true);
            return;
        }
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                          QStringLiteral("/ScreenSaver"),
                                                          QStringLiteral("org.freedesktop.ScreenSaver"),
                                                          QStringLiteral("Inhibit"));
        msg << QCoreApplication::applicationName() << parameters().value(QStringLiteral("reason")).toString();
        QDBusReply<uint> reply = QDBusConnection::sessionBus().call(msg);
        m_lockInhibitionCookie = reply.isValid() ? reply.value() : -1;
        setResult(reply.isValid());
        return;
    } else if (operation == QLatin1String("stopSuppressingScreenPowerManagement")) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                          QStringLiteral("/ScreenSaver"),
                                                          QStringLiteral("org.freedesktop.ScreenSaver"),
                                                          QStringLiteral("UnInhibit"));
        msg << m_lockInhibitionCookie;
        QDBusReply<uint> reply = QDBusConnection::sessionBus().call(msg);
        m_lockInhibitionCookie = reply.isValid() ? -1 : m_lockInhibitionCookie; // reset cookie if the stop request was successful
        setResult(reply.isValid());
        return;
    } else if (operation == QLatin1String("setPowerProfile")) {
        auto pending = setPowerProfile(parameters().value(QStringLiteral("profile")).toString());
        callWhenFinished(
            pending,
            [this](bool success) {
                setResult(success);
            },
            this);
        return;
    } else if (operation == QLatin1String("showPowerProfileOsd")) {
        QDBusMessage osdMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                             QStringLiteral("/org/kde/osdService"),
                                                             QStringLiteral("org.kde.osdService"),
                                                             QStringLiteral("powerProfileChanged"));
        osdMsg << parameters().value(QStringLiteral("profile")).toString();
        QDBusConnection::sessionBus().call(osdMsg);
    }

    qDebug() << "don't know what to do with " << operation;
    setResult(false);
}

QDBusPendingCall PowerManagementJob::setPowerProfile(const QString &value)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                      QStringLiteral("setProfile"));
    msg << value;
    return QDBusConnection::sessionBus().asyncCall(msg);
}
