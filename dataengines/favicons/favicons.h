/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2008 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2013 Andrea Scarpino <scarpino@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma/DataEngine>

class FaviconProvider;

/**
 * This class provides favicons for websites
 *
 * the queries are just the url of websites we want to fetch an icon
 */
class FaviconsEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    FaviconsEngine(QObject *parent, const QVariantList &args);
    ~FaviconsEngine() override;

protected:
    bool sourceRequestEvent(const QString &identifier) override;

protected Q_SLOTS:
    bool updateSourceEvent(const QString &identifier) override;

private Q_SLOTS:
    void finished(FaviconProvider *);
    void error(FaviconProvider *);
};
