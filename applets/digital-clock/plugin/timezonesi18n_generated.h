// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

// !!! This file was auto-generated. Do not edit it manually! !!!

#pragma once

#include <QHash>
#include <QString>

#include <KLocalizedString>

namespace TimezonesI18nData
{
using TimezoneContinentToL10nMap = QHash<QString, QString>;

static TimezoneContinentToL10nMap timezoneContinentToL10nMap()
{
    return {
        {QStringLiteral("Africa"), i18nc("This is a continent/area associated with a particular timezone", "Africa")},
        {QStringLiteral("America"), i18nc("This is a continent/area associated with a particular timezone", "America")},
        {QStringLiteral("Antarctica"), i18nc("This is a continent/area associated with a particular timezone", "Antarctica")},
        {QStringLiteral("Arctic"), i18nc("This is a continent/area associated with a particular timezone", "Arctic")},
        {QStringLiteral("Asia"), i18nc("This is a continent/area associated with a particular timezone", "Asia")},
        {QStringLiteral("Atlantic"), i18nc("This is a continent/area associated with a particular timezone", "Atlantic")},
        {QStringLiteral("Australia"), i18nc("This is a continent/area associated with a particular timezone", "Australia")},
        {QStringLiteral("Europe"), i18nc("This is a continent/area associated with a particular timezone", "Europe")},
        {QStringLiteral("Indian"), i18nc("This is a continent/area associated with a particular timezone", "Indian")},
        {QStringLiteral("Pacific"), i18nc("This is a continent/area associated with a particular timezone", "Pacific")},
    };
}
} // namespace TimezonesI18nData
