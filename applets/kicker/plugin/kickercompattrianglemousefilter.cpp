/*
 *    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>
 *
 *    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kickercompattrianglemousefilter.h"

#include "debug.h"

KickerCompatTriangleMouseFilter::KickerCompatTriangleMouseFilter(QQuickItem *parent)
    : TriangleMouseFilter(parent)
{
    qCWarning(KICKER_DEBUG) << "Kicker.TriangleMouseFilter is deprecated and will be removed in Plasma 6. Import TriangleMouseFilter from "
                               "org.kde.plasma.workspace.trianglemousefilter instead";
    setProperty("blockFirstEnter", true);
}
