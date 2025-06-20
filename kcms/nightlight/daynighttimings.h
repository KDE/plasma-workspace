/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KDarkLightScheduleProvider>

class DayNightTimings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(QDateTime morningStart READ morningStart NOTIFY morningStartChanged)
    Q_PROPERTY(QDateTime morningEnd READ morningEnd NOTIFY morningEndChanged)
    Q_PROPERTY(QDateTime eveningStart READ eveningStart NOTIFY eveningStartChanged)
    Q_PROPERTY(QDateTime eveningEnd READ eveningEnd NOTIFY eveningEndChanged)

public:
    explicit DayNightTimings(QObject *parent = nullptr);

    QDateTime dateTime() const;
    void setDateTime(const QDateTime &dateTime);

    QDateTime morningStart() const;
    QDateTime morningEnd() const;
    QDateTime eveningStart() const;
    QDateTime eveningEnd() const;

Q_SIGNALS:
    void dateTimeChanged();
    void morningStartChanged();
    void morningEndChanged();
    void eveningStartChanged();
    void eveningEndChanged();

private:
    void refresh();

    void setMorningStart(const QDateTime &dateTime);
    void setMorningEnd(const QDateTime &dateTime);
    void setEveningStart(const QDateTime &dateTime);
    void setEveningEnd(const QDateTime &dateTime);
    void setTransition(const KDarkLightTransition &transition);

    KDarkLightScheduleProvider *m_scheduleProvider;
    QDateTime m_dateTime;
    QDateTime m_morningStart;
    QDateTime m_morningEnd;
    QDateTime m_eveningStart;
    QDateTime m_eveningEnd;
};
