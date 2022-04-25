#include "clock.h"

// Note this class works against a single timezone, don't think about adding hours here!
class SystemTicker : public QObject
{
public:
    static QSharedPointer<SystemTicker> createTickerForInterval(int intervalMs);

private:
    SystemTicker(int intervalMs);
Q_SIGNALS:
    /**
     * This signal is fired every time either:
     *  - the interval fires
     *  - the underlying system clock changes
     */
    void tick();
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
