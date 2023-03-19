/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimeZone>

#include <memory>

#include <QQmlParserStatus>
#include <QtQml/qqmlregistration.h>

class AlignedTimer;

/**
 * Clock represents a time on a given timezone
 * Underneath Clock operates on a shared timer that is aligned to
 * update exactly on the second or minute (as appropriate)
 */
class Clock : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(Clock)

    Q_PROPERTY(QDateTime dateTime READ now NOTIFY timeChanged)
    Q_PROPERTY(bool trackSeconds READ isTrackSeconds WRITE setTrackSeconds NOTIFY trackSecondsChanged)

    Q_PROPERTY(QByteArray timeZone READ timeZone WRITE setTimeZone RESET resetTimeZone NOTIFY timeZoneChanged)
    Q_PROPERTY(bool isSystemTimeZone READ isSystemTimeZone NOTIFY timeZoneChanged)
    Q_PROPERTY(QString timeZoneCode READ timeZoneCode NOTIFY timeZoneChanged)
    Q_PROPERTY(QString timeZoneName READ timeZoneName NOTIFY timeZoneChanged)
    Q_PROPERTY(QString timeZoneOffset READ timeZoneOffset NOTIFY timeZoneChanged)

public:
    explicit Clock(QObject *parent = nullptr);
    /**
     * Returns the current time in the requested timezone
     */
    const QDateTime now() const;

    /**
     * @see setTrackSeconds
     */
    bool isTrackSeconds() const;
    /**
     * Sets whether we update every second or every aligned minute
     * Typically bound to whether we display seconds or not in our UI
     */
    void setTrackSeconds(bool trackSeconds);

    const QString dateFormat() const;
    /**
     * Sets the format string used to return formattedDate
     * The default is the user's locale's short format
     */
    void setDateFormat(const QString &newDateFormat);

    const QString timeFormat() const;
    /**
     * Sets the format string used to return formattedTime
     * The default is the user's locale's short format
     */
    void setTimeFormat(const QString &newTimeFormat);

    const QByteArray timeZone() const;
    /**
     * Sets the timezone to use for the time
     * The default is the user's system timezone
     *
     * Setting to undefined will return to the system default
     */
    void setTimeZone(const QByteArray &ianaId);
    void resetTimeZone();

    bool isSystemTimeZone() const;
    QString timeZoneCode() const;
    QString timeZoneName() const;
    QString timeZoneOffset() const;

    /**
     * Returns the current time as a string
     */
    const QString formattedTime() const;
    /**
     * Returns the current date as a string
     */
    const QString formattedDate() const;

    /**
     * @internal
     */
    void classBegin() override;
    /**
     * @internal
     */
    void componentComplete() override;
signals:
    void timeChanged();
    void timeZoneChanged();
    void trackSecondsChanged();

private Q_SLOTS:
    void handleSystemTimezoneChanged();

private:
    void update();
    void setupTickConnections();
    void setupTimeZone(const QTimeZone &timeZone);
    bool timezoneMetadataValid();

    bool m_deferInit = false;
    bool m_trackSeconds = false;
    bool m_timeZoneExplicitlySet = false;

    QString m_timeFormat;
    QString m_dateFormat;

    QTimeZone m_timeZone;
    QDateTime m_nextTimezoneTransition;
    QDateTime m_prevTimezoneTransition;

    std::shared_ptr<AlignedTimer> m_timer;
};
