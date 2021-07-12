/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "browser.h"
#include <QStringList>

class Favicon;

class Opera : public QObject, public Browser
{
    Q_OBJECT
public:
    explicit Opera(QObject *parent = nullptr);
    QList<BookmarkMatch> match(const QString &term, bool addEverything) override;
public Q_SLOTS:
    void prepare() override;
    void teardown() override;

private:
    QStringList m_operaBookmarkEntries;
    Favicon *const m_favicon;
};
