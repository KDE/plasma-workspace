/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

static const char CONFIG_USE_TRIGGERWORD[] = "useTriggerWord";
static const char CONFIG_TRIGGERWORD[] = "triggerWord";
static const char CONFIG_SORTING[] = "sorting";

/** Possibilities to sort */
enum Sort {
    NONE = 0,
    CPU,
    CPUI,
};
