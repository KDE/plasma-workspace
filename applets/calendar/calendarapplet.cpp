/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "calendarapplet.h"

#include <QDateTime>

CalendarApplet::CalendarApplet(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::Applet(parent, metaData, args)
{
}

CalendarApplet::~CalendarApplet()
{
}

int CalendarApplet::weekNumber(const QDateTime &dateTime) const
{
    return dateTime.date().weekNumber();
}

K_PLUGIN_CLASS_WITH_JSON(CalendarApplet, "metadata.json")

#include "calendarapplet.moc"
