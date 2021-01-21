/* Copyright 2009  <Jan Gerrit Marker> <jangerrit@weiler-marker.com>
 * Copyright 2020  <Alexander Lohnau> <alexander.lohnau@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
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
