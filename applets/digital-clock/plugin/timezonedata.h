/*
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QString>

class TimeZoneData
{
public:
    QString id;
    QString region;
    QString city;
    QString comment;
    bool checked = false;
    bool isLocalTimeZone = false;
    int offsetFromUtc = 0;
};
