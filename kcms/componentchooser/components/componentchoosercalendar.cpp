/*
    SPDX-FileCopyrightText: 2026 Denys Madureira <denys.madureira@pm.me>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "componentchoosercalendar.h"

ComponentChooserCalendar::ComponentChooserCalendar(QObject *parent)
    : ComponentChooser(parent, QStringLiteral("text/calendar"), QStringLiteral("Calendar"), QString(), i18n("Select default calendar application"))
{
}

QStringList ComponentChooserCalendar::mimeTypes() const
{
    return {QStringLiteral("text/calendar")};
}
