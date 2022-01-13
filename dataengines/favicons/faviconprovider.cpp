/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2008 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2013 Andrea Scarpino <scarpino@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
        Q_EMIT q->error(q);
        return;
    }

    image = QImage::fromData(job->data());
    if (!image.isNull()) {
        image.save(cachePath, "PNG");
    }
    Q_EMIT q->finished(q);
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
