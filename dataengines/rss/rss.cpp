/*
 *   Copyright (C) 2007 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

//Own
#include "rss.h"

//KDE
#include <QDebug>
#include <KUrl>
#include <KIO/FavIconRequestJob>
#include <Solid/Networking>
#include <syndication/item.h>
#include <syndication/loader.h>
#include <syndication/image.h>
#include <syndication/person.h>

//Plasma
#include <Plasma/DataEngine>

//Qt
#include <QDateTime>
#include <QDBusReply>
#include <QDBusInterface>
#include <QTimer>
#include <QSignalMapper>

#define TIMEOUT 15000    //timeout before updating the source if not all feeds
                         //are fetched.
#define CACHE_TIMEOUT 60 //time in seconds before the cached feeds are marked
                         //as out of date.
#define MINIMUM_INTERVAL 60000

RssEngine::RssEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args),
    m_forceUpdate(false)
{
    Q_UNUSED(args)
    setMinimumPollingInterval(MINIMUM_INTERVAL);
    m_signalMapper = new QSignalMapper(this);
    connect(m_signalMapper, SIGNAL(mapped(QString)),
            this, SLOT(timeout(QString)));
    connect(Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)),
            SLOT(networkStatusChanged(Solid::Networking::Status)));

}

RssEngine::~RssEngine()
{
}

void RssEngine::networkStatusChanged(Solid::Networking::Status status)
{
    if (status == Solid::Networking::Connected || status == Solid::Networking::Unknown) {
        qDebug() << "network connected, force refreshing feeds in 3 seconds";
        // The forced update needs to happen after the new feeds are in,
        // so remember to force the update in processRss()
        m_forceUpdate = true;
        // start updating the feeds
        foreach(const QString &feedUrl, sources()) {
            updateSourceEvent(feedUrl);
        }
    }
}

bool RssEngine::updateSourceEvent(const QString &name)
{
    /* Plasmoids using this engine should be able to retrieve
     * multiple feeds at the same time, so we allow a comma
     * separated list of url's
     */
    // NOTE: A comma separated list of feeds is not url compliant. Urls
    // may and do contain commas see http://www.spiegel.de/schlagzeilen/rss/0,5291,,00.xml
    // I have changed it to something more not url compliant " " three dots
    // Otherwise take a list instead
    const QStringList sources = name.split(' ', QString::SkipEmptyParts);

    foreach (const QString& source, sources) {
        setStorageEnabled(source, true);
        // Let's first see if we've got a recent cached version of
        // the feed. This avoids 'large' amounts of unnecessary network
        // traffic.
        if (QDateTime::currentDateTime() >
            m_feedTimes[source.toLower()].addSecs(CACHE_TIMEOUT)){
            qDebug() << "Cache from " << source <<
                        " older than 60 seconds, refreshing...";

            Syndication::Loader * loader = Syndication::Loader::create();
            connect(loader, SIGNAL(loadingComplete(Syndication::Loader*,
                                                   Syndication::FeedPtr,
                                                   Syndication::ErrorCode)),
                      this, SLOT(processRss(Syndication::Loader*,
                                            Syndication::FeedPtr,
                                            Syndication::ErrorCode)));

            m_feedMap.insert(loader, source);
            m_sourceMap.insert(loader, name);
            loader->loadFrom(source);
        } else {
            qDebug() << "Recent cached version of " << source <<
                        " found. Skipping...";

            // We might want to update the source:
            if (cachesUpToDate(name)) {
                updateFeeds(name, m_feedTitles[ source ] );
            }
        }
    }

    QTimer *timer = new QTimer(this);
    m_timerMap[name] = timer;
    timer->setSingleShot(true);
    m_signalMapper->setMapping(timer, name);

    connect(timer, SIGNAL(timeout()), m_signalMapper, SLOT(map()));

    timer->start(TIMEOUT);
    return true;
}

void RssEngine::slotFavIconResult(KJob *kjob)
{
    KIO::FavIconRequestJob *job = static_cast<KIO::FavIconRequestJob *>(kjob);
    const QString iconFile = job->iconFile();
    const QString url = job->hostUrl().toLower();

    m_feedIcons[url] = iconFile;
    QMap<QString, QVariant> map;

    for (int i = 0; i < m_feedItems[url].size(); i++) {
        map = m_feedItems[url].at(i).toMap();
        map["icon"] = iconFile;
        m_feedItems[url].replace(i, map);
    }

    //Are there sources ready to get updated now?
    foreach (const QString& source, m_sourceMap) {
        if (source.contains(url, Qt::CaseInsensitive) &&
            cachesUpToDate(source)) {
            qDebug() << "all caches from source " << source <<
                        " up to date, updating...";
            updateFeeds(source, m_feedTitles[ source ] );
        }
    }
}

void RssEngine::timeout(const QString & source)
{
    qDebug() << "timout fired, updating source";
    updateFeeds(source, m_feedTitles[ source ] );
    m_signalMapper->removeMappings(m_timerMap[source]);
}

bool RssEngine::sourceRequestEvent(const QString &name)
{
    setData(name, DataEngine::Data());
    updateSourceEvent(name);
    return true;
}

void RssEngine::processRss(Syndication::Loader* loader,
                           Syndication::FeedPtr feed,
                           Syndication::ErrorCode error)
{
    const QString url = m_feedMap.take(loader);
    const QString source = m_sourceMap.take(loader);
    QString title;
    bool iconRequested = false;
    KUrl u(url);

    if (error != Syndication::Success) {
        qDebug() << "Syndication did not work out... url = " << url;
        title = i18n("Syndication did not work out");
        setData(source, "title", i18n("Fetching feed failed."));
        setData(source, "link", url);
    } else {
        title = feed->title();
        QVariantList items;
        QString location;

        foreach (const Syndication::ItemPtr& item, feed->items()) {
            QMap<QString, QVariant> dataItem;

            //some malformed rss feeds can have empty entries
            if (item->title().isNull() && item->description().isNull() && dataItem["content"].isNull()) {
                continue;
            }

            dataItem["title"]       = item->title();
            dataItem["feed_title"]  = feed->title();
            dataItem["link"]        = item->link();
            dataItem["feed_url"]    = url;
            dataItem["description"] = item->description();
            dataItem["content"]     = item->content();
            dataItem["time"]        = (uint)item->dateUpdated();
            if (!m_feedIcons.contains(url.toLower()) && !iconRequested) {
                //lets request an icon, and only do this once per feed.
                KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(u);
                connect(job, &KIO::FavIconRequestJob::result, this, &RssEngine::slotFavIconResult);
                iconRequested = true;
            }
            dataItem["icon"] = m_feedIcons[url.toLower()];
            QStringList authors;
            foreach (const boost::shared_ptr<Syndication::Person> a, item->authors()) {
                authors << a->name();
            }
            dataItem["author"] = authors;

            items.append(dataItem);


            if (!m_rssSourceNames.contains(url)) {
                QMap<QString, QVariant> sourceItem;

                sourceItem["feed_title"] = feed->title();
                sourceItem["feed_url"] = url;
                sourceItem["icon"] = m_feedIcons[url.toLower()];

                m_rssSources.append(sourceItem);
                m_rssSourceNames.insert(url);
            }
        }
        m_feedItems[url.toLower()] = items;
        m_feedTimes[url.toLower()] = QDateTime::currentDateTime();
        m_feedTitles[url.toLower()] = title;

        // If we update the feeds every time a feed is fetched,
        // only the first update will actually update a connected
        // applet, which is actually sane, since plasma updates
        // only one time each interval. This means, however, that
        // we maybe want to delay updating the feeds untill either
        // timeout, or all feeds are up to date.
        if (cachesUpToDate(source)) {
            qDebug() << "all caches from source " << source
                     << " up to date, updating...";
            updateFeeds(source, title);
            if (m_forceUpdate) {
                // Should be used with care ...
                forceImmediateUpdateOfAllVisualizations();
                m_forceUpdate = false;
                // and skip scheduleSourcesUpdated(), since we
                // force a repaint anyway already
                return;
            }
        } else {
            qDebug() << "not all caches from source " << source
                     << ", delaying update.";
        }
        scheduleSourcesUpdated();
    }
}

void RssEngine::updateFeeds(const QString & source, const QString & title)
{
    /**
     * TODO: can this be improved? I'm calling mergeFeeds way too
     * often here...
     */
    const QVariantList list = mergeFeeds(source);
    setData(source, "items", list);

    setData(source, "sources", m_rssSources);
    const QStringList sourceNames = source.split(' ', QString::SkipEmptyParts);
    if (sourceNames.size() >  1) {
        setData(source, "title", i18np("1 RSS feed fetched",
                                       "%1 RSS feeds fetched", sourceNames.size()));
    } else {
        setData(source, "title", title);
    }
}

bool RssEngine::cachesUpToDate(const QString & source) const
{
    const QStringList sources = source.split(' ', QString::SkipEmptyParts);
    bool outOfDate = false;
    foreach (const QString &url, sources) {
        if (QDateTime::currentDateTime() >
            m_feedTimes[url.toLower()].addSecs(CACHE_TIMEOUT)){
            outOfDate = true;
        }
        if (!m_feedIcons.contains(url.toLower())) {
            outOfDate = true;
        }
    }
    return (!outOfDate);
}

bool compare(const QVariant &v1, const QVariant &v2)
{
     return v1.toMap()["time"].toUInt() > v2.toMap()["time"].toUInt();
}

QVariantList RssEngine::mergeFeeds(QString source) const
{
    QVariantList result;
    const QStringList sources = source.split(' ', QString::SkipEmptyParts);

    foreach (const QString& feed, sources) {
        result += m_feedItems[feed.toLower()];
    }

    qSort(result.begin(), result.end(), compare);
    return result;
}


