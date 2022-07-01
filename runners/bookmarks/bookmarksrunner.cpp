/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bookmarksrunner.h"
#include "browsers/browser.h"

#include <QDebug>
#include <QDir>
#include <QList>
#include <QStack>
#include <QUrl>

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KSharedConfig>

#include "bookmarkmatch.h"
#include "bookmarksrunner_defs.h"
#include "browserfactory.h"

K_PLUGIN_CLASS_WITH_JSON(BookmarksRunner, "plasma-runner-bookmarks.json")

BookmarksRunner::BookmarksRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
    , m_browser(nullptr)
    , m_browserFactory(new BrowserFactory(this))
{
    setObjectName(QStringLiteral("Bookmarks"));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds web browser bookmarks matching :q:.")));
    addSyntax(Plasma::RunnerSyntax(i18nc("list of all web browser bookmarks", "bookmarks"), i18n("List all web browser bookmarks")));

    connect(this, &Plasma::AbstractRunner::prepare, this, &BookmarksRunner::prep);
    setMinLetterCount(3);
}

BookmarksRunner::~BookmarksRunner() = default;

void BookmarksRunner::prep()
{
    auto browser = m_browserFactory->find(findBrowserName(), this);
    if (m_browser != browser) {
        m_browser = browser;
        connect(this, &Plasma::AbstractRunner::teardown, dynamic_cast<QObject *>(m_browser), [this]() {
            m_browser->teardown();
        });
    }
    m_browser->prepare();
}

void BookmarksRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    bool allBookmarks = term.compare(i18nc("list of all konqueror bookmarks", "bookmarks"), Qt::CaseInsensitive) == 0;

    const QList<BookmarkMatch> matches = m_browser->match(term, allBookmarks);
    for (BookmarkMatch match : matches) {
        if (!context.isValid())
            return;
        context.addMatch(match.asQueryMatch(this));
    }
}

QString BookmarksRunner::findBrowserName()
{
    const auto browser = KApplicationTrader::preferredService(QStringLiteral("x-scheme-handler/http"));

    if (browser) {
        return browser->exec();
    }

    return QString();
}

void BookmarksRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action)
{
    Q_UNUSED(context);
    const QString term = action.data().toString();
    QUrl url = QUrl(term);

    // support urls like "kde.org" by transforming them to https://kde.org
    if (url.scheme().isEmpty()) {
        const int idx = term.indexOf('/');

        url.clear();
        url.setHost(term.left(idx));
        if (idx != -1) {
            // allow queries
            const int queryStart = term.indexOf('?', idx);
            int pathLength = -1;
            if ((queryStart > -1) && (idx < queryStart)) {
                pathLength = queryStart - idx;
                url.setQuery(term.mid(queryStart));
            }

            url.setPath(term.mid(idx, pathLength));
        }
        url.setScheme(QStringLiteral("http"));
    }

    auto job = new KIO::OpenUrlJob(url);
    job->start();
}

#include "bookmarksrunner.moc"
