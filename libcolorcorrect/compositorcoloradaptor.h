/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QTime>

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

public:
    enum class ErrorCode {
        // no error
        ErrorCodeSuccess = 0,
        // couldn't establish connection to compositor
        ErrorCodeConnectionFailed,
        // rendering backend doesn't support hardware color correction
        ErrorCodeBackendNoSupport,
    };
    Q_ENUMS(ErrorCode)

    explicit CompositorAdaptor(QObject *parent = nullptr);
    ~CompositorAdaptor() override = default;

    int error() const
    {
        return (int)m_error;
    }
    void setError(ErrorCode error);

    QString errorText() const
    {
        return m_errorText;
    }

    bool nightColorAvailable() const
    {
        return m_nightColorAvailable;
    }

    bool running() const
    {
        return m_running;
    }

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
     * @brief Preview a color temperature for 15s.
     *
     * @return void
     * @since 5.25
     **/
    Q_INVOKABLE void preview(int temperature);

    /**
     * @brief Stop an ongoing preview.
     *
     * @return void
     * @since 5.25
     **/
    Q_INVOKABLE void stopPreview();

Q_SIGNALS:
    void errorChanged();
    void errorTextChanged();

    void runningChanged();

private:
    void updateProperties(const QVariantMap &properties);

    QDBusInterface *m_iface;

    ErrorCode m_error = ErrorCode::ErrorCodeSuccess;
    QString m_errorText;

    bool m_nightColorAvailable = false;

    bool m_running = false;

private Q_SLOTS:
    void handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
};

}
