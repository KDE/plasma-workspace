/* Copyright 2009  <Jan Gerrit Marker> <jangerrit@weiler-marker.com>
    Copyright 2020  <Alexander Lohnau> <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KILLRUNNER_CONFIG_KEYS_H
#define KILLRUNNER_CONFIG_KEYS_H

static const char CONFIG_USE_TRIGGERWORD[] = "useTriggerWord";
static const char CONFIG_TRIGGERWORD[] = "triggerWord";
static const char CONFIG_SORTING[] = "sorting";

/** Possibilities to sort */
enum Sort {
    NONE = 0,
    CPU,
    CPUI,
};

#endif
