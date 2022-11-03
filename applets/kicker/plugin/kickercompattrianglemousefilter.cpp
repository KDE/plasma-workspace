/*
 *    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>
 *
 *    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kickercompattrianglemousefilter.h"

KickerCompatTriangleMouseFilter::KickerCompatTriangleMouseFilter(QQuickItem *parent)
    : TriangleMouseFilter(parent)
{
    setProperty("blockFirstEnter", true);
}
