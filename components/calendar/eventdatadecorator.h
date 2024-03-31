/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

#include <CalendarEvents/CalendarEventsPlugin>

class EventDataDecorator
{
    Q_GADGET
    Q_PROPERTY(QDateTime startDateTime READ startDateTime CONSTANT FINAL)
    Q_PROPERTY(QDateTime endDateTime READ endDateTime CONSTANT FINAL)
    Q_PROPERTY(bool isAllDay READ isAllDay CONSTANT FINAL)
    Q_PROPERTY(bool isMinor READ isMinor CONSTANT FINAL)
    Q_PROPERTY(QString title READ title CONSTANT FINAL)
    Q_PROPERTY(QString description READ description CONSTANT FINAL)
    Q_PROPERTY(QString eventColor READ eventColor CONSTANT FINAL)
    Q_PROPERTY(QString eventType READ eventType CONSTANT FINAL)

public:
    explicit EventDataDecorator(const CalendarEvents::EventData &data);

    QDateTime startDateTime() const;
    QDateTime endDateTime() const;
    bool isAllDay() const;
    bool isMinor() const;
    QString title() const;
    QString description() const;
    QString eventType() const;
    QString eventColor() const;

private:
    CalendarEvents::EventData m_data;
};
