/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>
    SPDX-FileCopyrightText: 2016 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractItemModel>
#include <QPointer>

#include "daydata.h"
#include "eventpluginsmanager.h"
#include <CalendarEvents/CalendarEventsPlugin>

class DaysModelPrivate;

class DaysModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        isCurrent = Qt::UserRole + 1,
        // containsHolidayItems,
        containsEventItems,
        containsMajorEventItems,
        containsMinorEventItems,
        // containsTodoItems,
        // containsJournalItems,
        dayNumber,
        monthNumber,
        yearNumber,
        Events,
        EventColor,
        EventCount,
        AlternateDayNumber, /**< The day number from the alternate calendar system @since 5.26 */
        AlternateMonthNumber, /**< The month number from the alternate calendar system @since 5.26 */
        AlternateYearNumber, /**< The year number from the alternate calendar system @since 5.26 */
        SubLabel, /**< The label that is displayed in the tooltip or beside the full date @since 5.26 */
        SubDayLabel, /**< The label that is displayed under the day number @since 5.26 */
        SubMonthLabel, /**< The label that is displayed under the month number @since 5.26 */
        SubYearLabel, /**< The label that is displayed under the year number @since 5.26 */
    };

    explicit DaysModel(QObject *parent = nullptr);
    ~DaysModel() override;

    void setSourceData(QList<DayData> *data);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void setPluginsManager(QObject *manager);

    Q_INVOKABLE QList<QObject *> eventsForDate(const QDate &date);

    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void agendaUpdated(const QDate &updatedDate);

public Q_SLOTS:
    void update();

private Q_SLOTS:
    void onDataReady(const QMultiHash<QDate, CalendarEvents::EventData> &data);
    void onEventModified(const CalendarEvents::EventData &data);
    void onEventRemoved(const QString &uid);
    void onAlternateDateReady(const QHash<QDate, QDate> &data);
    void onSubLabelReady(const QHash<QDate, CalendarEvents::CalendarEventsPlugin::SubLabel> &data);

private:
    DaysModelPrivate *const d;

    QModelIndex indexForDate(const QDate &date);
    bool hasMajorEventAtDate(const QDate &date) const;
    bool hasMinorEventAtDate(const QDate &date) const;
};
