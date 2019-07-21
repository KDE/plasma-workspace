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
#ifndef COMPOSITORCOLORADAPTOR_H
#define COMPOSITORCOLORADAPTOR_H

#include <QObject>
#include <QTime>
#include <QHash>
#include <QString>

#include "colorcorrect_export.h"
#include "colorcorrectconstants.h"

class QDBusInterface;

namespace ColorCorrect
{

class COLORCORRECT_EXPORT CompositorAdaptor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

    Q_PROPERTY(bool nightColorAvailable READ nightColorAvailable CONSTANT)
    Q_PROPERTY(int minimalTemperature READ minimalTemperature CONSTANT)
    Q_PROPERTY(int neutralTemperature READ neutralTemperature CONSTANT)

    Q_PROPERTY(bool activeEnabled READ activeEnabled NOTIFY activeEnabledChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool activeStaged READ activeStaged WRITE setActiveStaged NOTIFY activeStagedChanged)
    Q_PROPERTY(bool activeDefault READ activeDefault CONSTANT)

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

    Q_PROPERTY(bool modeEnabled READ modeEnabled NOTIFY modeEnabledChanged)
    Q_PROPERTY(int mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(int modeStaged READ modeStaged WRITE setModeStaged NOTIFY modeStagedChanged)
    Q_PROPERTY(int modeDefault READ modeDefault CONSTANT)

    Q_PROPERTY(bool nightTemperatureEnabled READ nightTemperatureEnabled NOTIFY nightTemperatureEnabledChanged)
    Q_PROPERTY(int nightTemperature READ nightTemperature NOTIFY nightTemperatureChanged)
    Q_PROPERTY(int nightTemperatureStaged READ nightTemperatureStaged WRITE setNightTemperatureStaged NOTIFY nightTemperatureStagedChanged)
    Q_PROPERTY(int nightTemperatureDefault READ nightTemperatureDefault CONSTANT)

    Q_PROPERTY(int curColorT READ curColorT WRITE setCurColorT NOTIFY curColorTChanged)

    Q_PROPERTY(double latitudeAuto READ latitudeAuto NOTIFY latitudeAutoChanged)
    Q_PROPERTY(double longitudeAuto READ longitudeAuto NOTIFY longitudeAutoChanged)

    Q_PROPERTY(bool locationEnabled READ locationEnabled NOTIFY locationEnabledChanged)
    Q_PROPERTY(double latitudeFixed READ latitudeFixed NOTIFY latitudeFixedChanged)
    Q_PROPERTY(double longitudeFixed READ longitudeFixed NOTIFY longitudeFixedChanged)
    Q_PROPERTY(double latitudeFixedStaged READ latitudeFixedStaged WRITE setLatitudeFixedStaged NOTIFY latitudeFixedStagedChanged)
    Q_PROPERTY(double longitudeFixedStaged READ longitudeFixedStaged WRITE setLongitudeFixedStaged NOTIFY longitudeFixedStagedChanged)
    Q_PROPERTY(double latitudeFixedDefault READ latitudeFixedDefault CONSTANT)
    Q_PROPERTY(double longitudeFixedDefault READ longitudeFixedDefault CONSTANT)

    Q_PROPERTY(bool timingsEnabled READ timingsEnabled NOTIFY timingsEnabledChanged)

    Q_PROPERTY(QTime morningBeginFixed READ morningBeginFixed NOTIFY morningBeginFixedChanged)
    Q_PROPERTY(QTime morningBeginFixedStaged READ morningBeginFixedStaged WRITE setMorningBeginFixedStaged NOTIFY morningBeginFixedStagedChanged)
    Q_PROPERTY(QTime eveningBeginFixed READ eveningBeginFixed NOTIFY eveningBeginFixedChanged)
    Q_PROPERTY(QTime eveningBeginFixedStaged READ eveningBeginFixedStaged WRITE setEveningBeginFixedStaged NOTIFY eveningBeginFixedStagedChanged)
    Q_PROPERTY(int transitionTime READ transitionTime NOTIFY transitionTimeChanged)
    Q_PROPERTY(int transitionTimeStaged READ transitionTimeStaged WRITE setTransitionTimeStaged NOTIFY transitionTimeStagedChanged)
    Q_PROPERTY(QTime morningBeginFixedDefault READ morningBeginFixedDefault CONSTANT)
    Q_PROPERTY(QTime eveningBeginFixedDefault READ eveningBeginFixedDefault CONSTANT)
    Q_PROPERTY(int transitionTimeDefault READ transitionTimeDefault CONSTANT)

public:
    enum class ErrorCode {
        // no error
        ErrorCodeSuccess = 0,
        // couldn't establish connection to compositor
        ErrorCodeConnectionFailed,
        // rendering backend doesn't support hardware color correction
        ErrorCodeBackendNoSupport
    };
    Q_ENUMS(ErrorCode)

    enum class Mode {
        ModeAutomatic,
        ModeLocation,
        ModeTimings,
        ModeConstant,
    };
    Q_ENUMS(Mode)

    explicit CompositorAdaptor(QObject *parent = nullptr);
    ~CompositorAdaptor() override = default;

    int error() const {
        return (int)m_error;
    }
    void setError(ErrorCode error);

    QString errorText() const {
        return m_errorText;
    }

    /*
     * General
     */
    bool nightColorAvailable() const {
        return m_nightColorAvailable;
    }
    int minimalTemperature() const {
        return MIN_TEMPERATURE;
    }
    int neutralTemperature() const {
        return NEUTRAL_TEMPERATURE;
    }

    bool activeEnabled() const {
        return m_activeEnabled;
    }
    bool active() const {
        return m_active;
    }
    bool activeStaged() const {
        return m_activeStaged;
    }
    void setActiveStaged(bool set) {
        if (m_activeStaged == set) {
            return;
        }
        m_activeStaged = set;
        emit activeStagedChanged();
    }
    bool activeDefault() const {
        return true;
    }

    bool running() const {
        return m_running;
    }

    bool modeEnabled() const {
        return m_modeEnabled;
    }
    int mode() const {
        return (int)m_mode;
    }
    int modeStaged() const {
        return (int)m_modeStaged;
    }
    void setModeStaged(int mode) {
        if (mode < 0 || 3 < mode || (int)m_modeStaged == mode) {
            return;
        }
        m_modeStaged = (Mode)mode;
        emit modeStagedChanged();
    }
    int modeDefault() const {
        return (int)Mode::ModeAutomatic;
    }
    /*
     * Color Temperature
     */
    bool nightTemperatureEnabled() const {
        return m_nightTemperatureEnabled;
    }
    int nightTemperature() const {
        return m_nightTemperature;
    }
    int nightTemperatureStaged() const {
        return m_nightTemperatureStaged;
    }
    void setNightTemperatureStaged(int val) {
        if (m_nightTemperatureStaged == val) {
            return;
        }
        m_nightTemperatureStaged = val;
        emit nightTemperatureStagedChanged();
    }
    int nightTemperatureDefault() const {
        return DEFAULT_NIGHT_TEMPERATURE;
    }
    int curColorT() const {
        return m_curColorT;
    }
    void setCurColorT(int val) {
        if (m_nightTemperature == val) {
            return;
        }
        m_curColorT = val;
        emit curColorTChanged();
    }
    /*
     * Location
     */
    bool locationEnabled() const {
        return m_locationEnabled;
    }

    double latitudeAuto() const {
        return m_latitudeAuto;
    }
    double longitudeAuto() const {
        return m_longitudeAuto;
    }

    double latitudeFixed() const {
        return m_latitudeFixed;
    }
    double latitudeFixedStaged() const {
        return m_latitudeFixedStaged;
    }
    void setLatitudeFixedStaged(double val) {
        if (m_latitudeFixedStaged == val) {
            return;
        }
        m_latitudeFixedStaged = val;
        emit latitudeFixedStagedChanged();
    }

    double longitudeFixed() const {
        return m_longitudeFixed;
    }
    double longitudeFixedStaged() const {
        return m_longitudeFixedStaged;
    }
    void setLongitudeFixedStaged(double val) {
        if (m_longitudeFixedStaged == val) {
            return;
        }
        m_longitudeFixedStaged = val;
        emit longitudeFixedStagedChanged();
    }
    double latitudeFixedDefault() const {
        return 0.;
    }
    double longitudeFixedDefault() const {
        return 0.;
    }
    /*
     * Timings
     */
    bool timingsEnabled() const {
        return m_timingsEnabled;
    }

    QTime morningBeginFixed() const {
        return m_morningBeginFixed;
    }
    QTime morningBeginFixedStaged() const {
        return m_morningBeginFixedStaged;
    }
    void setMorningBeginFixedStaged(const QTime &time) {
        if (m_morningBeginFixedStaged == time) {
            return;
        }
        m_morningBeginFixedStaged = time;
        emit morningBeginFixedStagedChanged();
    }

    QTime eveningBeginFixed() const {
        return m_eveningBeginFixed;
    }
    QTime eveningBeginFixedStaged() const {
        return m_eveningBeginFixedStaged;
    }
    void setEveningBeginFixedStaged(const QTime &time) {
        if (m_eveningBeginFixedStaged == time) {
            return;
        }
        m_eveningBeginFixedStaged = time;
        emit eveningBeginFixedStagedChanged();
    }
    // saved in minutes
    int transitionTime() const {
        return m_transitionTime;
    }
    int transitionTimeStaged() const {
        return m_transitionTimeStaged;
    }
    void setTransitionTimeStaged(int time) {
        if (m_transitionTimeStaged == time) {
            return;
        }
        m_transitionTimeStaged = time;
        emit transitionTimeStagedChanged();
    }
    QTime morningBeginFixedDefault() const {
        return QTime(6,0,0);
    }
    QTime eveningBeginFixedDefault() const {
        return QTime(18,0,0);
    }
    int transitionTimeDefault() const {
        return FALLBACK_SLOW_UPDATE_TIME;
    }

    /**
     * @brief Reloads data and resets staged values.
     *
     * Reloads current data from compositor, also resets all staged values.
     * For data updates without resetting staged values, don't use this method
     * and instead connect to the compDataUpdated signal.
     *
     * @return void
     * @see compDataUpdated
     * @since 5.12
     **/
    Q_INVOKABLE void reloadData();
    /**
     * @brief Send subset of staged values.
     *
     * Send a relevant subset of staged values to the compositor in order
     * to trigger a configuration change. If active will be set to false, no
     * other data will be sent. Otherwise additionally staged temperature and
     * mode values will be sent and for the requested mode relevant data, i.e.
     * in Automatic mode no other data, in Location mode staged latitude or
     * longitude values and in Timings mode the morning and evening begin, as
     * well as the transition time.
     *
     * Returns true, if the configuration was successfully applied.
     *
     * @return bool
     * @see sendConfigurationAll
     * @since 5.12
     **/
    Q_INVOKABLE bool sendConfiguration();
    /**
     * @brief Send all staged values.
     *
     * Send all currently staged values to the compositor in order
     * to trigger a configuration change.
     *
     * Returns true, if the configuration was successfully applied.
     *
     * @return bool
     * @see sendConfiguration
     * @since 5.12
     **/
    Q_INVOKABLE bool sendConfigurationAll();
    /**
     * @brief Send automatic location data.
     *
     * Updated auto location data is provided by the workspace. This is
     * in general already done by the KDE Daemon.
     *
     * @return void
     * @since 5.12
     **/
    Q_INVOKABLE void sendAutoLocationUpdate(double latitude, double longitude);
    /**
     * @brief Check changes in subset of staged values.
     *
     * Compares staged to current values relative to chosen activation state and mode,
     * returns true if there is a difference.
     *
     * @return bool
     * @see checkStagedAll
     * @since 5.12
     **/
    Q_INVOKABLE bool checkStaged();
    /**
     * @brief Check changes in staged values.
     *
     * Compares every staged to its current value, returns true if there is a difference.
     *
     * @return bool
     * @see checkStaged
     * @since 5.12
     **/
    Q_INVOKABLE bool checkStagedAll();

private Q_SLOTS:
    void compDataUpdated(const QHash<QString, QVariant> &data);

Q_SIGNALS:
    void errorChanged();
    void errorTextChanged();

    void activeEnabledChanged();
    void activeChanged();
    void activeStagedChanged();

    void runningChanged();

    void modeEnabledChanged();
    void modeChanged();
    void modeStagedChanged();

    void curColorTChanged();
    void nightTemperatureEnabledChanged();
    void nightTemperatureChanged();
    void nightTemperatureStagedChanged();

    void latitudeAutoChanged();
    void longitudeAutoChanged();

    void locationEnabledChanged();
    void latitudeFixedChanged();
    void latitudeFixedStagedChanged();
    void longitudeFixedChanged();
    void longitudeFixedStagedChanged();

    void timingsEnabledChanged();
    void morningBeginFixedChanged();
    void eveningBeginFixedChanged();
    void morningBeginFixedStagedChanged();
    void eveningBeginFixedStagedChanged();

    void transitionTimeChanged();
    void transitionTimeStagedChanged();

    void dataUpdated();
    void stagedDataReset();


private:
    bool resetData(const QHash<QString, QVariant> &data);
    void resetDataAndStaged(const QHash<QString, QVariant> &data);
    QDBusInterface *m_iface;

    QHash<QString, QVariant> getData();

    ErrorCode m_error = ErrorCode::ErrorCodeSuccess;
    QString m_errorText;

    bool m_nightColorAvailable = false;

    bool m_activeEnabled = true;
    bool m_active = false;
    bool m_activeStaged = false;

    bool m_running = false;

    bool m_modeEnabled = true;
    Mode m_mode = Mode::ModeAutomatic;
    Mode m_modeStaged = Mode::ModeAutomatic;

    bool m_nightTemperatureEnabled = true;
    int m_nightTemperature = DEFAULT_NIGHT_TEMPERATURE;
    int m_nightTemperatureStaged = DEFAULT_NIGHT_TEMPERATURE;
    int m_curColorT;

    double m_latitudeAuto;
    double m_longitudeAuto;

    bool m_locationEnabled = true;
    double m_latitudeFixed = 0;
    double m_longitudeFixed = 0;
    double m_latitudeFixedStaged = 0;
    double m_longitudeFixedStaged = 0;

    bool m_timingsEnabled = true;

    QTime m_morningBeginFixed = QTime(6,0,0);
    QTime m_eveningBeginFixed = QTime(18,0,0);
    QTime m_morningBeginFixedStaged = QTime(6,0,0);
    QTime m_eveningBeginFixedStaged = QTime(18,0,0);

    int m_transitionTime = FALLBACK_SLOW_UPDATE_TIME;
    int m_transitionTimeStaged = FALLBACK_SLOW_UPDATE_TIME;
};

}

#endif // COMPOSITORCOLORADAPTOR_H
