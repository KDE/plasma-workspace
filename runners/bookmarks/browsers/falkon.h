/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
