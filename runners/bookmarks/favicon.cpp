/*
    SPDX-FileCopyrightText: 2007, 2012 Glenn Ergeerts <glenn.ergeerts@telenet.be>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "favicon.h"

Favicon::Favicon(QObject *parent)
    : QObject(parent)
    , m_default_icon(QIcon::fromTheme(QStringLiteral("bookmarks")))
{
}
