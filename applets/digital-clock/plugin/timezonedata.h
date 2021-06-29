/*
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TIMEZONEDATA_H
#define TIMEZONEDATA_H

#include <QString>

class TimeZoneData
{
public:
    QString id;
    QString region;
    QString city;
    QString comment;
    bool checked;
    bool isLocalTimeZone;
    int offsetFromUtc;
};

#endif // TIMEZONEDATA_H
