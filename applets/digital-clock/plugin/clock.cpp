#include "clock.h"

#include <QTimer>

/**
 * Singleton class that notifies every time a second or minute passes
 * Signals are emitted as close as possible to the system clock
 * Skews (e.g from suspend) are detected automatically
 */
// Note this class works against UTC, don't think about adding hoursChanged here
class SystemTicker : public QObject
{
public:
    static SystemTicker *instance();

Q_SIGNALS:
    void secondChanged();
    void minuteChanged();

protected:
    void connectNotify(const QMetaMethod &signal) override;
    void disconnectNotify(const QMetaMethod &signal) override;

private:
    void setupTimer();
    SystemTicker();
    QTimer m_timer;
    QDateTime m_lastTime;
    int secondsWatchingCount = 0;
    int minutesWatchingCount = 0;
};

Clock::Clock(QObject *parent)
    : QObject{parent}
{
    m_dateFormat = QLocale().dateFormat(QLocale::ShortFormat);
    m_timeFormat = QLocale().timeFormat(QLocale::ShortFormat);
}

const QString Clock::dateFormat() const
{
    return m_dateFormat;
}

void Clock::setDateFormat(const QString &newDateFormat)
{
    if (m_dateFormat == newDateFormat)
        return;
    m_dateFormat = newDateFormat;
    emit dateFormatChanged();
    emit dateChanged();
}

const QString Clock::timeFormat() const
{
    return m_timeFormat;
}

void Clock::setTimeFormat(const QString &newTimeFormat)
{
    if (m_timeFormat == newTimeFormat)
        return;
    m_timeFormat = newTimeFormat;
    // determine if we follow seconds?
    emit timeFormatChanged();
    emit timeChanged();
}

const QString Clock::timeZone() const
{
    return m_timeZone.id();
}

void Clock::setTimeZone(const QByteArray &ianaId)
{
    if (m_timeZone.id() == ianaId)
        return;
    m_timeZone = QTimeZone(ianaId);
    emit timeZoneChanged();

    onTick();
}

void Clock::resetTimeZone()
{
    m_timeZone = QTimeZone();
}

const QString Clock::formattedTime() const
{
    return now().toString(m_timeFormat);
}

const QString Clock::formattedDate() const
{
    return now().toString(m_dateFormat);
}

const QDateTime Clock::now() const
{
    // Our timers might be coarse and could fire early
    // If we add a small offset here we don't need to worry about any rounding when seconds etc. are floored
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc().addMSecs(200);
    return nowUtc.toTimeZone(m_timeZone);
}

void Clock::onTick()
{
    const QDateTime currentTime = now();
    if (currentTime.date() != m_lastTime.date()) {
        emit dateChanged();
    }
    emit timeChanged();
}
