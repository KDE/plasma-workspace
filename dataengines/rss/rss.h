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

#ifndef RSS_DATAENGINE_H
#define RSS_DATAENGINE_H

#include <syndication/item.h>
#include <syndication/loader.h>
#include <syndication/feed.h>
#include <Solid/Networking>
#include <Plasma/DataEngine>
#include <QSet>

class QDBusInterface;
class QSignalMapper;

/**
 * This class can be used to fetch one or more rss feeds. By
 * requesting a datasource with the url's of one or more rss
 * feeds separated by " ", you will receive a merged list
 * containing the items of all feeds requested. This list is
 * sorted by timestamp.
 * This class also fetches the favicons from all requested
 * feeds.
 */
class RssEngine : public Plasma::DataEngine
{
    Q_OBJECT

    public:
        RssEngine(QObject* parent, const QVariantList& args);
        virtual ~RssEngine();

    protected:
        bool sourceRequestEvent(const QString &name);
        bool updateSourceEvent(const QString& name);

    protected Q_SLOTS:
        void processRss(Syndication::Loader* loader,
                        Syndication::FeedPtr feed,
                        Syndication::ErrorCode error);
        void slotFavIconResult(KJob *job);
        void timeout(const QString & source);
        void networkStatusChanged(Solid::Networking::Status status);

    private:
        QVariantList mergeFeeds(QString source) const;
        void updateFeeds(const QString & source,
                         const QString & title);
        bool cachesUpToDate(const QString & source) const;

        QHash<Syndication::Loader*, QString> m_feedMap;
        QHash<Syndication::Loader*, QString> m_sourceMap;
        QHash<QString, QTimer*>              m_timerMap;
        QHash<QString, QVariantList>         m_feedItems;
        QHash<QString, QString>              m_feedIcons;
        QHash<QString, QString>              m_feedTitles;
        QHash<QString, QDateTime>            m_feedTimes;
        bool                                 m_forceUpdate;

        QVariantList                         m_rssSources;
        QSet<QString>                        m_rssSourceNames;

        QSignalMapper *                      m_signalMapper;
};

K_EXPORT_PLASMA_DATAENGINE(rss, RssEngine)

#endif

