/*
 * Copyright 2011 Sebastian Kügler <sebas@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

#include <Solid/PowerManagement>

PowerManagementJob::PowerManagementJob(const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent)
    : ServiceJob(parent->objectName(), operation, parameters, parent)
    , m_session(new SessionManagement(this))
{
}

PowerManagementJob::~PowerManagementJob()
{
}

static void callWhenFinished(const QDBusPendingCall& pending, std::function<void()> func, QObject* parent)
{
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(pending, parent);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                    parent, [func](QDBusPendingCallWatcher* watcher) {
                        watcher->deleteLater();
                        func();
                    });
}

void PowerManagementJob::start()
{
    const QString operation = operationName();
    //qDebug() << "starting operation  ... " << operation;

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
        setResult(Solid::PowerManagement::beginSuppressingSleep(parameters().value(QStringLiteral("reason")).toString()));
        return;
    } else if (operation == QLatin1String("stopSuppressingSleep")) {
        setResult(Solid::PowerManagement::stopSuppressingSleep(parameters().value(QStringLiteral("cookie")).toInt()));
        return;
    } else if (operation == QLatin1String("beginSuppressingScreenPowerManagement")) {
        setResult(Solid::PowerManagement::beginSuppressingScreenPowerManagement(parameters().value(QStringLiteral("reason")).toString()));
        return;
    } else if (operation == QLatin1String("stopSuppressingScreenPowerManagement")) {
        setResult(Solid::PowerManagement::stopSuppressingScreenPowerManagement(parameters().value(QStringLiteral("cookie")).toInt()));
        return;
    } else if (operation == QLatin1String("setBrightness")) {
        auto pending = setScreenBrightness(parameters().value(QStringLiteral("brightness")).toInt(), parameters().value(QStringLiteral("silent")).toBool());
        callWhenFinished(pending, [this] { setResult(true); }, this);
        return;
    } else if (operation == QLatin1String("setKeyboardBrightness")) {
        auto pending = setKeyboardBrightness(parameters().value(QStringLiteral("brightness")).toInt(), parameters().value(QStringLiteral("silent")).toBool());
        callWhenFinished(pending, [this] { setResult(true); }, this);
        return;
    }

    qDebug() << "don't know what to do with " << operation;
    setResult(false);
}

QDBusPendingCall PowerManagementJob::setScreenBrightness(int value, bool silent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                      silent ? "setBrightnessSilent" : "setBrightness");
    msg << value;
    return QDBusConnection::sessionBus().asyncCall(msg);
}

QDBusPendingCall PowerManagementJob::setKeyboardBrightness(int value, bool silent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                      silent ? "setKeyboardBrightnessSilent" : "setKeyboardBrightness");
    msg << value;
    return QDBusConnection::sessionBus().asyncCall(msg);
}
