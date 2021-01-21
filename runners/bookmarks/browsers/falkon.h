/*
 *   Copyright 2020 Alexander Lohnau <alexander.lohnau@gmx.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FALKON_H
#define FALKON_H

#include "browser.h"

class Favicon;

class Falkon : public QObject, public Browser
{
    Q_OBJECT
public:
    explicit Falkon(QObject *parent = nullptr);
    QList<BookmarkMatch> match(const QString &term, bool addEverything) override;
public Q_SLOTS:
    void prepare() override;
    void teardown() override;

private:
    QString getStartupProfileDir();
    QJsonArray m_falkonBookmarkEntries;
    QString m_startupProfile;
    Favicon *m_favicon;
};

#endif // FALKON_H
