#ifndef CLOCK_H
#define CLOCK_H

#include <QDateTime>
#include <QObject>
#include <QTimeZone>

/**
 * Clock represents a time on a given timezone
 * Underneath Clock operates on a shared timer that is aligned to
 * update exactly on the minute
 */
class Clock : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString timeZone READ timeZone WRITE setTimeZone RESET resetTimeZone NOTIFY timeZoneChanged)
    Q_PROPERTY(QString dateFormat READ dateFormat WRITE setDateFormat NOTIFY dateFormatChanged)
    Q_PROPERTY(QString timeFormat READ timeFormat WRITE setTimeFormat NOTIFY timeFormatChanged)
    Q_PROPERTY(QString formattedTime READ formattedTime NOTIFY timeChanged)
    Q_PROPERTY(QString formattedDate READ formattedDate NOTIFY dateChanged)
    Q_PROPERTY(QDateTime now READ now NOTIFY timeChanged)
    // we deliberately don't expose qdate and qtime separately as the conversion to JS objects goes badly

public:
    explicit Clock(QObject *parent = nullptr);

    const QString dateFormat() const;
    void setDateFormat(const QString &newDateFormat);

    const QString timeFormat() const;
    void setTimeFormat(const QString &newTimeFormat);

    const QString timeZone() const;
    /**
     * ianaId
     */
    void setTimeZone(const QByteArray &ianaId);
    void resetTimeZone();

    const QString formattedTime() const;
    const QString formattedDate() const;

    const QDateTime now() const;

signals:
    void dateFormatChanged();
    void timeFormatChanged();

    void timeZoneChanged();
    void timeChanged();
    void dateChanged();

private Q_SLOTS:
    void onTick();

private:
    void updateTickConnections();

    QString m_timeFormat;
    QString m_dateFormat;
    QTimeZone m_timeZone;
    QDateTime m_lastTime;
};

#endif // CLOCK_H
