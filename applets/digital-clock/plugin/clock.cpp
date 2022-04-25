#include "clock.h"

#include <QMetaMethod>
#include <QTimer>

/**
 * Singleton class that notifies every time a second or minute passes
 * Signals are emitted as close as possible to the system clock
 * Skews (e.g from suspend) are detected automatically
 *
 * This class is not thread safe
 */

// Note this class works against UTC, don't think about adding hoursChanged!
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

private Q_SLOTS:
    void onTimeout();
    void onClockSkew();

private:
    SystemTicker();
    void setupTimer();
    int targetInterval() const;

    QTimer m_timer;
    QDateTime m_lastTime;
    int m_secondsWatchingCount = 0;
    int m_minutesWatchingCount = 0;

    QMetaObject::Connection m_aligningConnection;
};

Clock::Clock(QObject *parent)
    : QObject{parent}
{
    m_dateFormat = QLocale().dateFormat(QLocale::ShortFormat);
    m_timeFormat = QLocale().timeFormat(QLocale::ShortFormat);
    onTick();
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

SystemTicker::SystemTicker()
{
    connect(&m_timer, &QTimer::timeout, this, &SystemTicker::onTimeout);

    //#ifdef Q_OS_LINUX
    //    // monitor for the system clock being changed
    //    auto timeChangedFd = timerfd_create(CLOCK_REALTIME, O_CLOEXEC | O_NONBLOCK);
    //    itimerspec timespec;
    //    memset(&timespec, 0, sizeof(timespec)); // set all timers to 0 seconds, which creates a timer that won't do anything

    //    int err = c(timeChangedFd, 3, &timespec, nullptr); // monitor for the time changing
    //    //(flags == TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET). However these are not exposed in glibc so value is hardcoded
    //    if (err) {
    //        qCWarning(DATAENGINE_TIME) << "Could not create timer with TFD_TIMER_CANCEL_ON_SET. Clock skews will not be detected. Error:"
    //                                   << qPrintable(strerror(err));
    //    }

    //    connect(this, &QObject::destroyed, [timeChangedFd]() {
    //        close(timeChangedFd);
    //    });

    //    auto notifier = new QSocketNotifier(timeChangedFd, QSocketNotifier::Read, this);
    //    connect(notifier, &QSocketNotifier::activated, this, [this](int fd) {
    //        uint64_t c;
    //        read(fd, &c, 8);
    //        clockSkewed();
    //    });
    //#else
    //    QDBusConnection dbus = QDBusConnection::sessionBus();
    //    dbus.connect(QString(), "/org/kde/kcmshell_clock", "org.kde.kcmshell_clock", "clockUpdated", this, SLOT(clockSkewed()));
    //    dbus.connect(QStringLiteral("org.kde.Solid.PowerManagement"),
    //                 QStringLiteral("/org/kde/Solid/PowerManagement/Actions/SuspendSession"),
    //                 QStringLiteral("org.kde.Solid.PowerManagement.Actions.SuspendSession"),
    //                 QStringLiteral("resumingFromSuspend"),
    //                 this,
    //                 SLOT(clockSkewed()));
    //#endif
}

void SystemTicker::connectNotify(const QMetaMethod &signal)
{
    int currentTarget = targetInterval();
    if (signal == QMetaMethod::fromSignal(&SystemTicker::secondChanged)) {
        m_secondsWatchingCount++;
    }
    if (signal == QMetaMethod::fromSignal(&SystemTicker::minuteChanged)) {
        m_minutesWatchingCount++;
    }
    if (currentTarget != targetInterval()) {
        setupTimer();
    }
}

void SystemTicker::disconnectNotify(const QMetaMethod &signal)
{
    int currentTarget = targetInterval();
    if (signal == QMetaMethod::fromSignal(&SystemTicker::secondChanged)) {
        m_secondsWatchingCount--;
    }
    if (signal == QMetaMethod::fromSignal(&SystemTicker::minuteChanged)) {
        m_minutesWatchingCount--;
    }
    setupTimer();
    if (currentTarget != targetInterval()) {
        setupTimer();
    }
}

void SystemTicker::onTimeout()
{
    QDateTime currentTime = QDateTime::currentDateTimeUtc();
    if (currentTime.time().second() != m_lastTime.time().second()) {
        Q_EMIT secondChanged();
    }
    if (currentTime.time().minute() != m_lastTime.time().minute()) {
        Q_EMIT minuteChanged();
    }
    m_lastTime = currentTime;
}

void SystemTicker::onClockSkew()
{
    onTimeout();
    setupTimer();
}

void SystemTicker::setupTimer()
{
    int interval = SystemTicker::targetInterval();
    if (interval == 0) {
        disconnect(m_aligningConnection);
        m_timer.stop();
        return;
    }

    // align to the relevant time, then start repeating
    disconnect(m_aligningConnection);
    m_timer.setSingleShot(true);
    int timeRemaining = interval - QDateTime::currentMSecsSinceEpoch() % interval;
    m_timer.setInterval(timeRemaining);

    m_aligningConnection = connect(&m_timer, &QTimer::timeout, this, [this, interval]() {
        disconnect(m_aligningConnection);
        m_timer.setSingleShot(false);
        m_timer.setInterval(interval);
    });
    m_timer.start();
}

int SystemTicker::targetInterval() const
{
    Q_ASSERT(m_secondsWatchingCount >= 0);
    Q_ASSERT(m_minutesWatchingCount >= 0);

    if (m_secondsWatchingCount == 0 && m_minutesWatchingCount == 0) {
        return 0;
    }
    if (m_secondsWatchingCount > 0) {
        return 1000 * 60;
    } else {
        return 1000;
    }
}
