/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Glenn Ergeerts <marco.gulino@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "favicon.h"

Favicon::Favicon(QObject *parent)
    : QObject(parent)
    , m_default_icon(QIcon::fromTheme(QStringLiteral("bookmarks")))
{
}
