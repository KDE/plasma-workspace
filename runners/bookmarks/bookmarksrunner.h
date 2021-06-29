/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Glenn Ergeerts <marco.gulino@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

private Q_SLOTS:
    void prep();
};

#endif
