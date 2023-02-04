/*
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QLocale>
#include <QString>

class TimeZoneData
{
public:
    QString id;
    QString region;
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    QLocale::Territory territory;
#else
    QLocale::Country territory;
#endif
    QString city;
    QString comment;
    bool checked = false;
    bool isLocalTimeZone = false;
    int offsetFromUtc = 0;
};
