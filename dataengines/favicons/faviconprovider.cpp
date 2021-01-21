/*
 *   Copyright (C) 2007 Tobias Koenig <tokoe@kde.org>
 *   Copyright (C) 2008 Marco Martin <notmart@gmail.com>
 *   Copyright (C) 2013 Andrea Scarpino <scarpino@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
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

#include "faviconprovider.h"

#include <QImage>
#include <QStandardPaths>
#include <QUrl>

#include <KIO/Job>
#include <KIO/StoredTransferJob>
#include <KJob>

class FaviconProvider::Private
{
public:
    Private(FaviconProvider *parent)
        : q(parent)
    {
    }

    void imageRequestFinished(KIO::StoredTransferJob *job);

    FaviconProvider *q;
    QImage image;
    QString cachePath;
};

void FaviconProvider::Private::imageRequestFinished(KIO::StoredTransferJob *job)
{
    if (job->error()) {
        emit q->error(q);
        return;
    }

    image = QImage::fromData(job->data());
    if (!image.isNull()) {
        image.save(cachePath, "PNG");
    }
    emit q->finished(q);
}

FaviconProvider::FaviconProvider(QObject *parent, const QString &url)
    : QObject(parent)
    , m_url(url)
    , d(new Private(this))
{
    QUrl faviconUrl = QUrl::fromUserInput(url);
    const QString fileName = KIO::favIconForUrl(faviconUrl);

    if (!fileName.isEmpty()) {
        d->cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + '/' + fileName + ".png";
        d->image.load(d->cachePath, "PNG");
    } else {
        d->cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/favicons/" + faviconUrl.host() + ".png";
        faviconUrl.setPath(QStringLiteral("/favicon.ico"));

        if (faviconUrl.isValid()) {
            KIO::StoredTransferJob *job = KIO::storedGet(faviconUrl, KIO::NoReload, KIO::HideProgressInfo);
            // job->setProperty("uid", id);
            connect(job, &KJob::result, this, [this, job]() {
                d->imageRequestFinished(job);
            });
        }
    }
}

FaviconProvider::~FaviconProvider()
{
    delete d;
}

QImage FaviconProvider::image() const
{
    return d->image;
}

QString FaviconProvider::identifier() const
{
    return m_url;
}

#include "moc_faviconprovider.cpp"
