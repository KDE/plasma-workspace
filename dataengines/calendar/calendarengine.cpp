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


#include "calendarengine.h"

#include <QtCore/QDate>

#include <KCalendarSystem>
#include <KDateTime>
#include <KSystemTimeZones>
#include <KHolidays/HolidayRegion>

#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KCalCore/Journal>

#ifdef AKONADI_FOUND
#include "eventdatacontainer.h"
#endif

CalendarEngine::CalendarEngine(QObject* parent, const QVariantList& args)
              : Plasma::DataEngine(parent)
{
    Q_UNUSED(args);
}

CalendarEngine::~CalendarEngine()
{
    qDeleteAll(m_regions);
}

bool CalendarEngine::sourceRequestEvent(const QString &request)
{
    qDebug() << "Request = " << request << '\n';

    if (request.isEmpty()) {
        return false;
    }

    QStringList requestTokens = request.split(':');
    QString requestKey = requestTokens.takeFirst();

    if (requestKey == "holidaysRegions" ||
        requestKey == "holidaysRegion" ||
        requestKey == "holidaysDefaultRegion" ||
        requestKey == "holidaysIsValidRegion" ||
        requestKey == "holidays" ||
        requestKey == "holidaysInMonth") {
        return holidayCalendarSourceRequest(requestKey, requestTokens, request);
    }

#ifdef AKONADI_FOUND
    if (requestKey == "events" || requestKey == "eventsInMonth") {
        return akonadiCalendarSourceRequest(requestKey, requestTokens, request);
    }
#endif

    return false;
}

bool CalendarEngine::holidayCalendarSourceRequest(const QString& key, const QStringList& args, const QString& request)
{
    if (key == "holidaysRegions") {
        QStringList regionList = KHolidays::HolidayRegion::regionCodes();
        Plasma::DataEngine::Data data;
        foreach (const QString &regionCode, regionList) {
            Plasma::DataEngine::Data regionData;
            KHolidays::HolidayRegion region(regionCode);
            regionData.insert("Name", region.name());
            regionData.insert("Description", region.description());
            regionData.insert("CountryCode", region.countryCode());
            regionData.insert("Location", region.location());
            regionData.insert("LanguageCode", region.languageCode());
            data.insert(regionCode, regionData);
        }
        setData(request, data);
       return true;
    }

    if (key == "holidaysDefaultRegion") {
        // If not set or the locale has changed since last set, then try determine a default region.
        if(m_defaultHolidayRegion.isEmpty() ||
           m_defaultHolidayRegionCountry != KLocale::global()->country() ||
           m_defaultHolidayRegionLanguage != KLocale::global()->language()) {

            m_defaultHolidayRegion = QString();
            m_defaultHolidayRegionCountry = KLocale::global()->country();
            m_defaultHolidayRegionLanguage = KLocale::global()->language();

            m_defaultHolidayRegion = KHolidays::HolidayRegion::defaultRegionCode(
                                                        m_defaultHolidayRegionCountry.toLower(),
                                                        m_defaultHolidayRegionLanguage.toLower() );
            if (m_defaultHolidayRegion.isEmpty()) {
                m_defaultHolidayRegion = "NoDefault";
            }
        }

        if (m_defaultHolidayRegion == "NoDefault") {
            setData(request, QString());
        } else {
            setData(request, m_defaultHolidayRegion);
        }
        return true;
    }


    int argsCount = args.count();
    if (argsCount < 1) {
        return false;
    }

    const QStringList regionCodeList = args.at(0).split(',');
    if (regionCodeList.count() < 1) {
        return false;
    }

    foreach ( const QString &regionCode, regionCodeList ) {
        KHolidays::HolidayRegion *region = m_regions.value(regionCode);
        if (!region || !region->isValid()) {
            region = new KHolidays::HolidayRegion(regionCode);
            if (region->isValid()) {
                m_regions.insert(regionCode, region);
            } else {
                delete region;
                return false;
            }
        }
    }

    if (key == "holidaysIsValidRegion") {
        if (regionCodeList.count() > 1) {
            return false;
        }
        QString regionCode = regionCodeList.at(0);
        if (m_regions.contains(regionCode)) {
            setData(request, m_regions.value(regionCode)->isValid());
        } else {
            setData(request, KHolidays::HolidayRegion::isValid(regionCode));
        }
        return true;
    }

    if (key == "holidaysRegion") {
        Plasma::DataEngine::Data data;
        foreach (const QString &regionCode, regionCodeList) {
            Plasma::DataEngine::Data regionData;
            KHolidays::HolidayRegion *region = m_regions.value(regionCode);
            regionData.insert("Name", region->name());
            regionData.insert("Description", region->description());
            regionData.insert("CountryCode", region->countryCode());
            regionData.insert("Location", region->location());
            regionData.insert("LanguageCode", region->languageCode());
            data.insert(regionCode, regionData);
        }
        setData(request, data);
        return true;
    }

    if (argsCount < 2) {
        return false;
    }

    QDate dateArg = QDate::fromString(args.at(1), Qt::ISODate);
    if (!dateArg.isValid()) {
        return false;
    }

    if (key == "holidaysInMonth" || key == "holidays") {
        QDate startDate, endDate;
        if (key == "holidaysInMonth") {
            int requestYear, requestMonth;
            KLocale::global()->calendar()->getDate(dateArg, &requestYear, &requestMonth, 0);
            int lastDay = KLocale::global()->calendar()->daysInMonth(dateArg);
            KLocale::global()->calendar()->setDate(startDate, requestYear, requestMonth, 1);
            KLocale::global()->calendar()->setDate(endDate, requestYear, requestMonth, lastDay);
        } else if (argsCount == 2) {
            startDate = dateArg;
            endDate = dateArg;
        } else if (argsCount < 3) {
            return false;
        } else {
            startDate = dateArg;
            endDate = QDate::fromString(args.at(2), Qt::ISODate);
        }

        if (!startDate.isValid() || !endDate.isValid()) {
            return false;
        }

        QList<QVariant> holidayList;
        foreach ( const QString &regionCode, regionCodeList ) {
            KHolidays::HolidayRegion *region = m_regions.value(regionCode);
            KHolidays::Holiday::List holidays;
            holidays = region->holidays(startDate, endDate, KHolidays::Holiday::MultidayHolidaysAsSingleEvents);

            foreach (const KHolidays::Holiday &holiday, holidays) {
                if (!holiday.text().isEmpty()) {
                    Plasma::DataEngine::Data holidayData;
                    holidayData.insert("Name", holiday.text());
                    holidayData.insert("RegionCode", regionCode);
                    holidayData.insert("ObservanceStartDate", holiday.observedStartDate().toString(Qt::ISODate));
                    holidayData.insert("ObservanceEndDate", holiday.observedEndDate().toString(Qt::ISODate));
                    holidayData.insert("ObservanceDuration", holiday.duration());
                    // It's a blunt tool for now, we only know if it's a full public holiday or not
                    if ( holiday.dayType() == KHolidays::Holiday::NonWorkday ) {
                        holidayData.insert("ObservanceType", "PublicHoliday");
                    } else {
                        holidayData.insert("ObservanceType", "Other");
                    }
                    holidayList.append(QVariant(holidayData));
                }
            }
        }

        setData(request, QVariant(holidayList));
        return true;
    }

    if (key == "isHoliday") {
        bool isHoliday = false;
        foreach ( const QString &regionCode, regionCodeList ) {
            KHolidays::HolidayRegion *region = m_regions.value(regionCode);
            if (region->isHoliday(dateArg)) {
                isHoliday = true;
            }
        }
        setData(request, isHoliday);
        return true;
    }

    if (key == "description") {
        QString summary;
        foreach ( const QString &regionCode, regionCodeList ) {
            KHolidays::HolidayRegion *region = m_regions.value(regionCode);
            KHolidays::Holiday::List holidays;
            holidays = region->holidays(dateArg, KHolidays::Holiday::MultidayHolidaysAsSingleEvents);
            foreach (const KHolidays::Holiday &holiday, holidays) {
                if (!summary.isEmpty()) {
                    summary.append("\n");
                }
                summary.append(holiday.text());
            }
        }

        setData(request, summary);
        return true;
    }

    return false;
}

#ifdef AKONADI_FOUND
bool CalendarEngine::akonadiCalendarSourceRequest(const QString& key, const QStringList& args, const QString& request)
{
    // figure out what time range was requested from the source string
    QDate start;
    QDate end;
    if (key == "eventsInMonth") {
        if (args.count() < 1) {
            return false;
        }
        start = QDate::fromString(args.at(0), Qt::ISODate);
        start.setDate(start.year(), start.month(), 1);
        end = QDate(start.year(), start.month(), start.daysInMonth());
    } else if (key == "events") {
        if (args.count() == 1) {
            start = QDate::fromString(args.at(0), Qt::ISODate);
            end = start.addDays(1);
        } else {
            if (args.count() < 2) {
                return false;
            }
            start = QDate::fromString(args.at(0), Qt::ISODate);
            end = QDate::fromString(args.at(1), Qt::ISODate);
        }
    } else {
        return false;
    }

    if (!start.isValid() || !end.isValid()) {
        return false;
    }

    if (!m_calendar) {
        m_calendar = Akonadi::ETMCalendar::Ptr(new Akonadi::ETMCalendar());
        m_calendar->setCollectionFilteringEnabled(false);
    }

    // create the corresponding EventDataContainer
    addSource(new EventDataContainer(m_calendar, request, KDateTime(start, QTime(0, 0, 0)), KDateTime(end, QTime(23, 59, 59))));
    return true;
}

#endif // AKONADI_FOUND


