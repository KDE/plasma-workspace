/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>
    SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

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

#include <defaultservice.h>

#include "bookmarkmatch.h"
#include "bookmarksrunner_defs.h"
#include "browsers/browser.h"
#include "browsers/chrome.h"
#include "browsers/chromefindprofile.h"
#include "browsers/falkon.h"
#include "browsers/firefox.h"
#include "browsers/konqueror.h"
#include "browsers/opera.h"

K_PLUGIN_CLASS_WITH_JSON(BookmarksRunner, "plasma-runner-bookmarks.json")

BookmarksRunner::BookmarksRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    addSyntax(QStringLiteral(":q:"), i18n("Finds web browser bookmarks matching :q:."));
    addSyntax(i18nc("list of all web browser bookmarks", "bookmarks"), i18n("List all web browser bookmarks"));

    connect(this, &KRunner::AbstractRunner::prepare, this, &BookmarksRunner::prep);
}

void BookmarksRunner::prep()
{
    if (!m_browser || m_currentBrowserName != findBrowserName()) {
        m_browser = findBrowser(findBrowserName());
        connect(this, &KRunner::AbstractRunner::teardown, this, [this]() {
            m_browser->teardown();
        });
    }
    m_browser->prepare();
}

void BookmarksRunner::match(KRunner::RunnerContext &context)
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
    const auto service = DefaultService::browser();
    return service ? service->exec() : DefaultService::legacyBrowserExec();
}

void BookmarksRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &action)
{
    const QString term = action.data().toString();
    QUrl url = QUrl(term);

    // support urls like "kde.org" by transforming them to https://kde.org
    if (url.scheme().isEmpty()) {
        const int idx = term.indexOf(u'/');

        url.clear();
        url.setHost(term.left(idx));
        if (idx != -1) {
            // allow queries
            const int queryStart = term.indexOf(u'?', idx);
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

std::unique_ptr<Browser> BookmarksRunner::findBrowser(const QString &browserName)
{
    if (browserName.contains(QLatin1String("firefox"), Qt::CaseInsensitive) || browserName.contains(QLatin1String("iceweasel"), Qt::CaseInsensitive)) {
        return std::make_unique<Firefox>(QDir::homePath() + QStringLiteral("/.mozilla/firefox"));
    } else if (browserName.contains(QLatin1String("opera"), Qt::CaseInsensitive)) {
        return std::make_unique<Opera>();
    } else if (browserName.contains(QLatin1String("chrome"), Qt::CaseInsensitive)) {
        return std::make_unique<Chrome>(new FindChromeProfile(QStringLiteral("google-chrome"), QDir::homePath()));
    } else if (browserName.contains(QLatin1String("chromium"), Qt::CaseInsensitive)) {
        return std::make_unique<Chrome>(new FindChromeProfile(QStringLiteral("chromium"), QDir::homePath()));
    } else if (browserName.contains(QLatin1String("falkon"), Qt::CaseInsensitive)) {
        return std::make_unique<Falkon>();
    } else {
        return std::make_unique<Konqueror>();
    }
}

#include "bookmarksrunner.moc"

#include "moc_bookmarksrunner.cpp"
