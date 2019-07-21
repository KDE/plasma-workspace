/********************************************************************
Copyright 2017 Roman Gilg <subdiff@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "compositorcoloradaptor.h"

#include <KLocalizedString>

#include <QDBusReply>
#include <QDBusInterface>

namespace ColorCorrect
{

CompositorAdaptor::CompositorAdaptor(QObject *parent)
    : QObject(parent)
{
    m_iface = new QDBusInterface (QStringLiteral("org.kde.KWin"),
                                    QStringLiteral("/ColorCorrect"),
                                    QStringLiteral("org.kde.kwin.ColorCorrect"),
                                    QDBusConnection::sessionBus(),
                                    this);

    if (!m_iface->connection().connect(QString(), QStringLiteral("/ColorCorrect"),
                                          QStringLiteral("org.kde.kwin.ColorCorrect"), QStringLiteral("nightColorConfigChanged"),
                                          this, SLOT(compDataUpdated(QHash<QString, QVariant>)))) {
        setError(ErrorCode::ErrorCodeConnectionFailed);
        return;
    }
    reloadData();
}

void CompositorAdaptor::setError(ErrorCode error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    switch(error) {
    case ErrorCode::ErrorCodeConnectionFailed:
        m_errorText = i18nc("Critical error message", "Failed to connect to the Window Manager");
        break;
    case ErrorCode::ErrorCodeBackendNoSupport:
        m_errorText = i18nc("Critical error message", "Rendering backend doesn't support Color Correction.");
        break;
    default:
        m_errorText = "";
    }
    emit errorChanged();
    emit errorTextChanged();
}

void CompositorAdaptor::compDataUpdated(const QHash<QString, QVariant> &data)
{
    resetData(data);
    emit dataUpdated();
}

void CompositorAdaptor::reloadData()
{
    QHash<QString, QVariant> info = getData();
    if (info.isEmpty()) {
        return;
    }
    resetDataAndStaged(info);
}

#define SETTER(out, in, emitsignal) \
if (out != in) { \
    out = in; \
    emit emitsignal; \
}

void CompositorAdaptor::resetDataAndStaged(const QHash<QString, QVariant> &data)
{
    if (!resetData(data)) {
        return;
    }

    SETTER(m_activeStaged, m_active, activeStagedChanged())
    SETTER(m_modeStaged, m_mode, modeStagedChanged())

    SETTER(m_nightTemperatureStaged, m_nightTemperature, nightTemperatureStagedChanged())

    SETTER(m_latitudeFixedStaged, m_latitudeFixed, latitudeFixedStagedChanged())
    SETTER(m_longitudeFixedStaged, m_longitudeFixed, longitudeFixedStagedChanged())

    SETTER(m_morningBeginFixedStaged, m_morningBeginFixed, morningBeginFixedStagedChanged())
    SETTER(m_eveningBeginFixedStaged, m_eveningBeginFixed, eveningBeginFixedStagedChanged())
    SETTER(m_transitionTimeStaged, m_transitionTime, transitionTimeStagedChanged())

    emit stagedDataReset();
}

bool CompositorAdaptor::resetData(const QHash<QString, QVariant> &data)
{
    m_nightColorAvailable = data["Available"].toBool();

    if (!m_nightColorAvailable) {
        setError(ErrorCode::ErrorCodeBackendNoSupport);
        return false;
    }

    SETTER(m_activeEnabled, data["ActiveEnabled"].toBool(), activeEnabledChanged())
    SETTER(m_active, data["Active"].toBool(), activeChanged())

    SETTER(m_running, data["Running"].toBool(), runningChanged())
    SETTER(m_mode, (Mode)data["Mode"].toInt(), modeChanged())

    SETTER(m_nightTemperatureEnabled, data["NightTemperatureEnabled"].toBool(), nightTemperatureEnabledChanged())
    SETTER(m_nightTemperature, data["NightTemperature"].toInt(), nightTemperatureChanged())
    SETTER(m_curColorT, data["CurrentColorTemperature"].toInt(), curColorTChanged())

    SETTER(m_locationEnabled, data["LocationEnabled"].toBool(), locationEnabledChanged())
    SETTER(m_latitudeAuto, data["LatitudeAuto"].toDouble(), latitudeAutoChanged())
    SETTER(m_longitudeAuto, data["LongitudeAuto"].toDouble(), longitudeAutoChanged())
    SETTER(m_latitudeFixed, data["LatitudeFixed"].toDouble(), latitudeFixedChanged())
    SETTER(m_longitudeFixed, data["LongitudeFixed"].toDouble(), longitudeFixedChanged())

    SETTER(m_timingsEnabled, data["TimingsEnabled"].toBool(), timingsEnabledChanged())
    SETTER(m_morningBeginFixed, QTime::fromString(data["MorningBeginFixed"].toString()), morningBeginFixedChanged())
    SETTER(m_eveningBeginFixed, QTime::fromString(data["EveningBeginFixed"].toString()), eveningBeginFixedChanged())
    SETTER(m_transitionTime, data["TransitionTime"].toInt(), transitionTimeChanged())

    return true;
}

#undef SETTER

QHash<QString, QVariant> CompositorAdaptor::getData()
{
    QDBusReply<QHash<QString, QVariant> > reply = m_iface->call("nightColorInfo");
    if (reply.isValid()) {
        return reply.value();
    } else {
        setError(ErrorCode::ErrorCodeConnectionFailed);
        return QHash<QString, QVariant>();
    }
}

bool CompositorAdaptor::sendConfiguration()
{
    if (!m_iface) {
        return false;
    }
    QHash<QString, QVariant> data;
    data["Active"] = m_activeStaged;

    if (m_activeStaged) {
        data["Mode"] = (int)m_modeStaged;

        data["NightTemperature"] = m_nightTemperatureStaged;

        if (m_modeStaged == Mode::ModeLocation) {
            data["LatitudeFixed"] = m_latitudeFixedStaged;
            data["LongitudeFixed"] = m_longitudeFixedStaged;
        }

        if (m_modeStaged == Mode::ModeTimings) {
            data["MorningBeginFixed"] = m_morningBeginFixedStaged.toString(Qt::ISODate);
            data["EveningBeginFixed"] = m_eveningBeginFixedStaged.toString(Qt::ISODate);
            data["TransitionTime"] = m_transitionTimeStaged;
        }
    }

    QDBusReply<bool> reply = m_iface->call("setNightColorConfig", data);
    if (reply.isValid() && reply) {
        m_active = m_activeStaged;
        if (m_activeStaged) {
            m_mode = m_modeStaged;
            m_nightTemperature = m_nightTemperatureStaged;

            if (m_mode == Mode::ModeLocation) {
                m_latitudeFixed = m_latitudeFixedStaged;
                m_longitudeFixed = m_longitudeFixedStaged;
            }

            if (m_mode == Mode::ModeTimings) {
                m_morningBeginFixed = m_morningBeginFixedStaged;
                m_eveningBeginFixed = m_eveningBeginFixedStaged;
                m_transitionTime = m_transitionTimeStaged;
            }
        }
        return true;
    }
    return false;
}

bool CompositorAdaptor::sendConfigurationAll()
{
    if (!m_iface) {
        return false;
    }
    QHash<QString, QVariant> data;
    data["Active"] = m_activeStaged;
    data["Mode"] = (int)m_modeStaged;
    data["NightTemperature"] = m_nightTemperatureStaged;
    data["LatitudeFixed"] = m_latitudeFixedStaged;
    data["LongitudeFixed"] = m_longitudeFixedStaged;
    data["MorningBeginFixed"] = m_morningBeginFixedStaged.toString(Qt::ISODate);
    data["EveningBeginFixed"] = m_eveningBeginFixedStaged.toString(Qt::ISODate);
    data["TransitionTime"] = m_transitionTimeStaged;

    QDBusReply<bool> reply = m_iface->call("setNightColorConfig", data);
    if (reply.isValid() && reply) {
        m_active = m_activeStaged;
        m_mode = m_modeStaged;
        m_nightTemperature = m_nightTemperatureStaged;

        m_latitudeFixed = m_latitudeFixedStaged;
        m_longitudeFixed = m_longitudeFixedStaged;

        m_morningBeginFixed = m_morningBeginFixedStaged;
        m_eveningBeginFixed = m_eveningBeginFixedStaged;
        m_transitionTime = m_transitionTimeStaged;
        return true;
    }
    return false;
}

void CompositorAdaptor::sendAutoLocationUpdate(double latitude, double longitude)
{
    if (m_mode == Mode::ModeAutomatic) {
        m_iface->call("nightColorAutoLocationUpdate", latitude, longitude);
    }
}

bool CompositorAdaptor::checkStaged()
{
    bool actChange = m_active != m_activeStaged;
    if (!m_activeStaged) {
        return actChange;
    }

    bool baseDataChange = actChange ||
            m_mode != m_modeStaged ||
            m_nightTemperature != m_nightTemperatureStaged;
    switch(m_modeStaged) {
    case Mode::ModeAutomatic:
        return baseDataChange;
    case Mode::ModeLocation:
        return baseDataChange ||
                m_latitudeFixed != m_latitudeFixedStaged ||
                m_longitudeFixed != m_longitudeFixedStaged;
    case Mode::ModeTimings:
        return baseDataChange ||
                m_morningBeginFixed != m_morningBeginFixedStaged ||
                m_eveningBeginFixed != m_eveningBeginFixedStaged ||
                m_transitionTime != m_transitionTimeStaged;
    case Mode::ModeConstant:
        return baseDataChange;
    default:
        // never reached
        return false;
    }
}

bool CompositorAdaptor::checkStagedAll()
{
    return m_active != m_activeStaged ||
            m_mode != m_modeStaged ||
            m_nightTemperature != m_nightTemperatureStaged ||
            m_latitudeFixed != m_latitudeFixedStaged ||
            m_longitudeFixed != m_longitudeFixedStaged ||
            m_morningBeginFixed != m_morningBeginFixedStaged ||
            m_eveningBeginFixed != m_eveningBeginFixedStaged ||
            m_transitionTime != m_transitionTimeStaged;
}

}
