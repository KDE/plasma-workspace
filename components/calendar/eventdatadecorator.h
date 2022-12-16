/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

#include <CalendarEvents/CalendarEventsPlugin>

class EventDataDecorator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDateTime startDateTime READ startDateTime NOTIFY eventDataChanged)
    Q_PROPERTY(QDateTime endDateTime READ endDateTime NOTIFY eventDataChanged)
    Q_PROPERTY(bool isAllDay READ isAllDay NOTIFY eventDataChanged)
    Q_PROPERTY(bool isMinor READ isMinor NOTIFY eventDataChanged)
    Q_PROPERTY(QString title READ title NOTIFY eventDataChanged)
    Q_PROPERTY(QString description READ description NOTIFY eventDataChanged)
    Q_PROPERTY(QString eventColor READ eventColor NOTIFY eventDataChanged)
    Q_PROPERTY(QString eventType READ eventType NOTIFY eventDataChanged)

public:
    EventDataDecorator(const CalendarEvents::EventData &data, QObject *parent = nullptr);

    QDateTime startDateTime() const;
    QDateTime endDateTime() const;
    bool isAllDay() const;
    bool isMinor() const;
    QString title() const;
    QString description() const;
    QString eventType() const;
    QString eventColor() const;

Q_SIGNALS:
    void eventDataChanged();

private:
    CalendarEvents::EventData m_data;
};
