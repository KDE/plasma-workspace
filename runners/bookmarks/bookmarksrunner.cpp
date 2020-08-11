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

#include "bookmarksrunner.h"
#include "browsers/browser.h"

#include <QList>
#include <QStack>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <QDesktopServices>

#include <KLocalizedString>
#include <KApplicationTrader>
#include <KSharedConfig>

#include "bookmarkmatch.h"
#include "browserfactory.h"
#include "bookmarksrunner_defs.h"

K_EXPORT_PLASMA_RUNNER_WITH_JSON(BookmarksRunner, "plasma-runner-bookmarks.json")


BookmarksRunner::BookmarksRunner( QObject* parent, const QVariantList &args )
    : Plasma::AbstractRunner(parent, args), m_browser(nullptr), m_browserFactory(new BrowserFactory(this))
{
    setObjectName(QStringLiteral("Bookmarks"));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds web browser bookmarks matching :q:.")));
    setDefaultSyntax(Plasma::RunnerSyntax(i18nc("list of all web browser bookmarks", "bookmarks"),
                                   i18n("List all web browser bookmarks")));

    connect(this, &Plasma::AbstractRunner::prepare, this, &BookmarksRunner::prep);
}

BookmarksRunner::~BookmarksRunner() = default;

void BookmarksRunner::prep()
{
    auto browser = m_browserFactory->find(findBrowserName(), this);
    if (m_browser != browser) {
        m_browser = browser;
        connect(this, &Plasma::AbstractRunner::teardown,
                dynamic_cast<QObject*>(m_browser), [this] () { m_browser->teardown(); });
    }
    m_browser->prepare();
}

void BookmarksRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    if ((term.length() < 3) && (!context.singleRunnerQueryMode())) {
        return;
    }

    bool allBookmarks = term.compare(i18nc("list of all konqueror bookmarks", "bookmarks"),
                                     Qt::CaseInsensitive) == 0;
                                     
    const QList<BookmarkMatch> matches = m_browser->match(term, allBookmarks);
    for(BookmarkMatch match : matches) {
        if(!context.isValid())
            return;
        context.addMatch(match.asQueryMatch(this));
    }
}

QString BookmarksRunner::findBrowserName()
{
    //HACK find the default browser
    KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General") );
    QString exec = config.readPathEntry(QStringLiteral("BrowserApplication"), QString());
    //qDebug() << "Found exec string: " << exec;
    if (exec.isEmpty()) {
        KService::Ptr service = KApplicationTrader::preferredService(QStringLiteral("text/html"));
        if (service) {
            exec = service->exec();
        }
    }

    //qDebug() << "KRunner::Bookmarks: found executable " << exec << " as default browser";
    return exec;

}

void BookmarksRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action)
{
    Q_UNUSED(context);
    const QString term = action.data().toString();
    QUrl url = QUrl(term);

    //support urls like "kde.org" by transforming them to https://kde.org
    if (url.scheme().isEmpty()) {
        const int idx = term.indexOf('/');

        url.clear();
        url.setHost(term.left(idx));
        if (idx != -1) {
            //allow queries
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

    QDesktopServices::openUrl(url);
}

QMimeData * BookmarksRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    QMimeData * result = new QMimeData();
    QList<QUrl> urls;
    urls << QUrl(match.data().toString());
    result->setUrls(urls);

    result->setText(match.data().toString());

    return result;
}

#include "bookmarksrunner.moc"
