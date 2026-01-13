/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QMimeData>
#include <krunner/abstractrunner.h>

class Browser;

/** This runner searches for bookmarks in browsers like Konqueror, Firefox and Opera */
class BookmarksRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    BookmarksRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

private:
    /** @returns the browser to get the bookmarks from
     * @see Browser
     */
    QString findBrowserName();

private:
    static std::unique_ptr<Browser> findBrowser(const QString &browserName);

    std::unique_ptr<Browser> m_browser;
    QString m_currentBrowserName;

private Q_SLOTS:
    void prep();
};
