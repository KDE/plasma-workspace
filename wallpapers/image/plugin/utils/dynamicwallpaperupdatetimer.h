/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DYNAMICWALLPAPERUPDATETIMER_H
#define DYNAMICWALLPAPERUPDATETIMER_H

#include <QDateTime>
#include <QTimer>

#include "finder/imagepackage.h"

class ClockSkewNotifierEngine;

/**
 * A timer that helps control the timed dynamic wallpapers
 */
class DynamicWallpaperUpdateTimer : public QTimer
{
    Q_OBJECT

public:
    explicit DynamicWallpaperUpdateTimer(KPackage::ImagePackage *package, QObject *parent = nullptr);

    /**
     * Use this function instead of start/stop to update ClockSkewNotifierEngine
     *
     * @param active @c true if the timer should be activated, @c false otherwise.
     */
    void setActive(bool active);

public Q_SLOTS:
    void alignInterval();
    void slotPrepareForSleep(bool sleep);

Q_SIGNALS:
    void clockSkewed();

private:
    ClockSkewNotifierEngine *m_engine = nullptr;
    KPackage::ImagePackage *m_imagePackage = nullptr;

    DynamicMetadataTimeInfoList m_intervals;
    quint64 m_totalTime = -1;
    QDateTime m_startTime;
};

#endif // DYNAMICWALLPAPERUPDATETIMER_H
