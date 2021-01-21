/*
 * Copyright 2008  Alex Merry <alex.merry@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "playeractionjob.h"

#include <dbusproperties.h>
#include <mprisplayer.h>
#include <mprisroot.h>

#define TRANSLATION_DOMAIN "plasma_engine_mpris2"
#include <KLocalizedString>

#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>

#include "debug.h"

PlayerActionJob::PlayerActionJob(const QString &operation, QMap<QString, QVariant> &parameters, PlayerControl *parent)
    : ServiceJob(parent->name() + ": " + operation, operation, parameters, parent)
    , m_controller(parent)
{
}

void PlayerActionJob::start()
{
    const QString operation(operationName());

    if (!m_controller) {
        setError(Failed);
        emitResult();
        return;
    }

    qCDebug(MPRIS2) << "Trying to perform the action" << operationName();
    if (!m_controller->isOperationEnabled(operation)) {
        setError(Denied);
        emitResult();
        return;
    }

    if (operation == QLatin1String("Quit") || operation == QLatin1String("Raise") || operation == QLatin1String("SetFullscreen")) {
        listenToCall(m_controller->rootInterface()->asyncCall(operation));
    } else if (operation == QLatin1String("Play") || operation == QLatin1String("Pause") || operation == QLatin1String("PlayPause")
               || operation == QLatin1String("Stop") || operation == QLatin1String("Next") || operation == QLatin1String("Previous")) {
        listenToCall(m_controller->playerInterface()->asyncCall(operation));
    } else if (operation == QLatin1String("Seek")) {
        if (parameters().value(QStringLiteral("microseconds")).canConvert<long long>()) {
            listenToCall(m_controller->playerInterface()->Seek(parameters()[QStringLiteral("microseconds")].toLongLong()));
        } else {
            setErrorText(QStringLiteral("microseconds"));
            setError(MissingArgument);
            emitResult();
        }
    } else if (operation == QLatin1String("SetPosition")) {
        if (parameters().value(QStringLiteral("microseconds")).canConvert<long long>()) {
            listenToCall(m_controller->playerInterface()->SetPosition(m_controller->trackId(), parameters()[QStringLiteral("microseconds")].toLongLong()));
        } else {
            setErrorText(QStringLiteral("microseconds"));
            setError(MissingArgument);
            emitResult();
        }
    } else if (operation == QLatin1String("OpenUri")) {
        if (parameters().value(QStringLiteral("uri")).canConvert<QUrl>()) {
            listenToCall(m_controller->playerInterface()->OpenUri(QString::fromLatin1(parameters()[QStringLiteral("uri")].toUrl().toEncoded())));
        } else {
            qCDebug(MPRIS2) << "uri was of type" << parameters().value(QStringLiteral("uri")).userType();
            setErrorText(QStringLiteral("uri"));
            setError(MissingArgument);
            emitResult();
        }
    } else if (operation == QLatin1String("SetLoopStatus")) {
        if (parameters().value(QStringLiteral("status")).type() == QVariant::String) {
            setDBusProperty(m_controller->playerInterface()->interface(), QStringLiteral("LoopStatus"), QDBusVariant(parameters()[QStringLiteral("status")]));
        } else {
            setErrorText(QStringLiteral("status"));
            setError(MissingArgument);
            emitResult();
        }
    } else if (operation == QLatin1String("SetShuffle")) {
        if (parameters().value(QStringLiteral("on")).type() == QVariant::Bool) {
            setDBusProperty(m_controller->playerInterface()->interface(), QStringLiteral("Shuffle"), QDBusVariant(parameters()[QStringLiteral("on")]));
        } else {
            setErrorText(QStringLiteral("on"));
            setError(MissingArgument);
            emitResult();
        }
    } else if (operation == QLatin1String("SetRate")) {
        if (parameters().value(QStringLiteral("rate")).type() == QVariant::Double) {
            setDBusProperty(m_controller->playerInterface()->interface(), QStringLiteral("Rate"), QDBusVariant(parameters()[QStringLiteral("rate")]));
        } else {
            setErrorText(QStringLiteral("rate"));
            setError(MissingArgument);
            emitResult();
        }
    } else if (operation == QLatin1String("SetVolume")) {
        if (parameters().value(QStringLiteral("level")).type() == QVariant::Double) {
            setDBusProperty(m_controller->playerInterface()->interface(), QStringLiteral("Volume"), QDBusVariant(parameters()[QStringLiteral("level")]));
        } else {
            setErrorText(QStringLiteral("level"));
            setError(MissingArgument);
            emitResult();
        }
    } else if (operation == QLatin1String("ChangeVolume")) {
        if (parameters().value(QStringLiteral("delta")).type() != QVariant::Double) {
            setErrorText(QStringLiteral("delta"));
            setError(MissingArgument);
            emitResult();
            return;
        }
        if (parameters().value(QStringLiteral("showOSD")).type() != QVariant::Bool) {
            setErrorText(QStringLiteral("showOSD"));
            setError(MissingArgument);
            emitResult();
            return;
        }

        m_controller->changeVolume(parameters()[QStringLiteral("delta")].toDouble(), parameters()[QStringLiteral("showOSD")].toBool());
        setError(NoError);
        emitResult();
    } else if (operation == QLatin1String("GetPosition")) {
        m_controller->updatePosition();
    } else {
        setError(UnknownOperation);
        emitResult();
    }
}

void PlayerActionJob::listenToCall(const QDBusPendingCall &call)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &PlayerActionJob::callFinished);
}

void PlayerActionJob::callFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<void> result = *watcher;
    watcher->deleteLater();

    if (result.isError()) {
        // FIXME: try to be a bit cleverer with the error message?
        setError(Failed);
        setErrorText(result.error().message());
    } else {
        setError(NoError);
    }

    emitResult();
}

void PlayerActionJob::setDBusProperty(const QString &iface, const QString &propName, const QDBusVariant &value)
{
    if (!m_controller) {
        setError(Failed);
        emitResult();
        return;
    }

    listenToCall(m_controller->propertiesInterface()->Set(iface, propName, value));
}

QString PlayerActionJob::errorString() const
{
    if (error() == Denied) {
        const QString name = m_controller ? m_controller->name() : QString();
        return i18n("The media player '%1' cannot perform the action '%2'.", name, operationName());
    } else if (error() == Failed) {
        return i18n("Attempting to perform the action '%1' failed with the message '%2'.", operationName(), errorText());
    } else if (error() == MissingArgument) {
        return i18n("The argument '%1' for the action '%2' is missing or of the wrong type.", operationName(), errorText());
    } else if (error() == UnknownOperation) {
        return i18n("The operation '%1' is unknown.", operationName());
    }
    return i18n("Unknown error.");
}

// vim: sw=4 sts=4 et tw=100
