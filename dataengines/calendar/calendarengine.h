/*
    Copyright (c) 2009 Davide Bettio <davide.bettio@kdemail.net>
    Copyright (c) 2010 Frederik Gladhorn <gladhorn@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/


#ifndef CALENDARENGINE_H
#define CALENDARENGINE_H

#include <Plasma/DataEngine>

#ifdef AKONADI_FOUND
# include <Akonadi/Calendar/ETMCalendar>
#endif

namespace KHolidays
{
    class HolidayRegion;
} // namespace KHolidays

/**
    The calendar data engine delivers calendar events.
    It can be queried for holidays or the akonadi calendar.

    Supported Holiday requests are:

        holidaysRegions
            * Returns a list of available Holiday Region Codes
        holidaysDefaultRegion
            * Returns a QString of a sensible default Holiday Region
        holdaysIsValidRegion:[regionCode]
            * Returns a bool if given Holiday Region is valid
        holdaysRegion:[regionCode(s)]
            * Returns the details of the Holiday Regions
        holidays:[regionCode(s)]:[YYYY-MM-DD]:[YYYY-MM-DD]
            * Returns a QList of all holidays in a Holiday Region(s) between two given dates.
        holidays:[regionCode(s)]:[YYYY-MM-DD]
            * Returns a QList of all holidays  in a Holiday Region(s) on a given day
        holidaysInMonth:[regionCode(s)]:[YYYY-MM-DD]
            * Returns a QList of all holidays in a Holiday Region(s) in a given month
        isHoliday:[regionCode(s)]:[YYYY-MM-DD]
            * Returns a bool if a given date is a Holiday in the given Holiday Region(s)
        description:[regionCode(s)]:[YYYY-MM-DD]
            * Returns a QString of all holiday names in a given Holiday Region(s) on a given date

    Where valid, regionCode(s) are a comma separated list of one or more valid Holiday Regions

    Each Holiday Region is a pair of a QString containing the regionCode and a Data containing value
    pairs for:
        "Name"                      Name of the Holiday Region          String
        "Description"               The description  of the Region      String
        "CountryCode"               The country code of the Region      String, ISO 3166-2 format
        "Location"                  The location of the Region          String, ISO 3166-1 format
        "LanguageCode"              The language of the Region          String, ISO 639-1 format

    Each Holiday is a Data containing QString value pairs for:
        "Name"                      Name of holiday                     String
        "RegionCode"                The Holiday Region code             String
        "ObservanceStartDate"       The start date of the holiday       ISO format Gregorian date
        "ObservanceEndDate"         The end date of the holiday         ISO format Gregorian date
        "ObservanceDuration"        How many days the holiday lasts     Integer
        "ObservanceType"            If the holiday is a day off         "PublicHoliday", "Other"

    Note that multiple holidays can be returned for each date.

    Supported Akonadi requests are:

        eventsInMonth:[YYYY-MM-DD]
        events:[YYYY-MM-DD]:[YYYY-MM-DD]
        events:[YYYY-MM-DD]

        The returned data contains (not all fields guaranteed to be populated):

            "UID"                     QString
            "Type"                    QString        "Event", "Todo", Journal"
            "Summary"                 QString
            "Comments"                QStringList
            "Location"                QString
            "OrganizerName"           QString
            "OrganizerEmail"          QString
            "Priority"                int
            "StartDate"               KDateTime
            "EndDate"                 KDateTime
            "RecurrenceDates"         QList(QVariant(KDateTime))
            "Recurs"                  bool
            "AllDay"                  bool
            "Categories"              QStringList
            "Resources"               QStringList
            "DurationDays"            int
            "DurationSeconds"         int
            "Status"                  QString         "None", "Tentative", "Confirmed", "Draft",
                                                      "Final", "Completed", "InProcess",
                                                      "Cancelled", "NeedsAction", "NonStandard",
                                                      "Unknown"
            "StatusName"              QString         translated Status
            "Secrecy"                 QString         "Public", "Private", "Confidential", "Unknown"
            "SecrecyName"             QString         translated Secrecy
            "Occurrences"             QList(QVariant(Plasma::DataEngine::Data))
                where each Data contains details of an occurence of the event:
                    "OccurrenceUid"          QString      for convenience, same as UID
                    "OccurrenceStartDate"    KDateTime
                    "OccurrenceEndDate"      KDateTime

        Event type specific data keys:
            "EventMultiDay"           bool
            "EventHasEndDate"         bool
            "EventTransparency"       QString         "Opaque", "Transparent", "Unknown"

        Todo type specific data keys:
            "TodoHasStartDate"        bool
            "TodoIsOpenEnded"         bool
            "TodoHasDueDate"          bool
            "TodoDueDate"             KDateTime
            "TodoIsCompleted"         bool
            "TodoIsInProgress"        bool
            "TodoIsNotStarted"        bool
            "TodoPercentComplete"     int
            "TodoHasCompletedDate"    bool
            "TodoCompletedDate"       bool

        Fields still to be done:
            Attendees
            Attachments
            Relations
            Alarms
            Custom Properties
            Lat/Lon
            Collection/Source

*/
class CalendarEngine : public Plasma::DataEngine
{
    Q_OBJECT

    public:
        CalendarEngine( QObject* parent, const QVariantList& args );
        ~CalendarEngine();

    protected:
        /// general request for data from this data engine
        bool sourceRequestEvent(const QString &name);

    private:
        /// a request for holidays data
        bool holidayCalendarSourceRequest(const QString& key, const QStringList& args, const QString& request);

        /// a request for data that comes from akonadi
        /// creates EventDataContainers as needed
        bool akonadiCalendarSourceRequest(const QString& key, const QStringList& args, const QString& request);

#ifdef AKONADI_FOUND
        /// this is the representation of the root calendar itself. it contains everything (calendars, incidences)
        Akonadi::ETMCalendar::Ptr m_calendar;
#endif

        /// holiday calendar
        QHash<QString, KHolidays::HolidayRegion *> m_regions;
        QString m_defaultHolidayRegion; // Cached value of default holiday region
        QString m_defaultHolidayRegionCountry; // The locale country when the cached default calculated
        QString m_defaultHolidayRegionLanguage; // The locale language when the cached default calculated
};

K_EXPORT_PLASMA_DATAENGINE(calendar, CalendarEngine)

#endif
