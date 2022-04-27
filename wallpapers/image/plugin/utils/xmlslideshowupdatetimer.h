/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SLIDESHOWUPDATETIMER_H
#define SLIDESHOWUPDATETIMER_H

#include <QDateTime>
#include <QTimer>

class ClockSkewNotifierEngine;
class SlideshowData;

using TimeList = std::vector<std::pair<int /* type */, qint64>>;
using TimeListPair = std::pair<TimeList, qint64 /* total time */>;

/**
 * A timer that controls the XML slideshow progress
 */
class XmlSlideshowUpdateTimer : public QTimer
{
    Q_OBJECT

public:
    XmlSlideshowUpdateTimer(QObject *parent = nullptr);

    /**
     * Use this function instead of start/stop to update ClockSkewNotifierEngine
     *
     * @param active @c true if the timer should be activated, @c false otherwise.
     */
    void setActive(bool active);

    void adjustInterval(const QString &xmlpath);

    static QDateTime slideshowStartTime(const SlideshowData &sData);
    static TimeListPair slideshowTimeList(const SlideshowData &sData);

    bool isTransition = false;

public Q_SLOTS:
    void alignInterval();

Q_SIGNALS:
    void clockSkewed();

private:
    ClockSkewNotifierEngine *m_engine = nullptr;

    std::vector<std::pair<int /* type */, qint64>> m_intervals;
    qint64 m_totalTime = -1;
    QDateTime m_startTime;
};

#endif // SLIDESHOWUPDATETIMER_H
