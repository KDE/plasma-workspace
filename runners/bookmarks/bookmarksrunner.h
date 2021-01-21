/*
 *   Copyright 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
 *   Copyright 2012 Glenn Ergeerts <marco.gulino@gmail.com>
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

#ifndef BOOKMARKSRUNNER_H
#define BOOKMARKSRUNNER_H

#include <QMimeData>
#include <krunner/abstractrunner.h>

class Browser;
class BrowserFactory;

/** This runner searchs for bookmarks in browsers like Konqueror, Firefox and Opera */
class BookmarksRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    BookmarksRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~BookmarksRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

private:
    /** @returns the browser to get the bookmarks from
     * @see Browser
     */
    QString findBrowserName();

private:
    Browser *m_browser;
    BrowserFactory *const m_browserFactory;
protected Q_SLOTS:
    QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match) override;

private Q_SLOTS:
    void prep();
};

#endif
