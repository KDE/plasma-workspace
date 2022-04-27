/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "xmlimageprovider.h"

#include <QFileInfo>
#include <QPainter>
#include <QUrlQuery>

#include "finder/xmlfinder.h"
#include "xmlslideshowupdatetimer.h"

class AsyncXmlImageResponseRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit AsyncXmlImageResponseRunnable(const QString &path, const QSize &requestedSize);

    /**
     * Read the image and resize it if the requested size is valid.
     */
    void run() override;

Q_SIGNALS:
    void done(const QImage &image);

private:
    void blendImages(QImage &from, QImage &to, double toOpacity) const;

    QString m_path;
    QSize m_requestedSize;
};

class AsyncXmlImageResponse : public QQuickImageResponse
{
    Q_OBJECT

public:
    explicit AsyncXmlImageResponse(const QString &path, const QSize &requestedSize, QThreadPool *pool);

    QQuickTextureFactory *textureFactory() const override;

protected Q_SLOTS:
    void slotHandleDone(const QImage &image);

protected:
    QImage m_image;
};

AsyncXmlImageResponseRunnable::AsyncXmlImageResponseRunnable(const QString &path, const QSize &requestedSize)
    : m_path(path)
    , m_requestedSize(requestedSize)
{
}

void AsyncXmlImageResponseRunnable::run()
{
    const QUrlQuery urlQuery(QUrl(QStringLiteral("image://gnome-wp-list/%1").arg(m_path)));

    const QString filename = urlQuery.queryItemValue(QStringLiteral("filename"));
    const QString filename_dark = urlQuery.queryItemValue(QStringLiteral("filename_dark"));
    const bool useDark = urlQuery.queryItemValue(QStringLiteral("darkmode")).toInt() == 1;

    QString path = filename;
    const QFileInfo info(filename);

    if (!info.exists()) {
        Q_EMIT done(QImage());
        return;
    }

    if (useDark && !filename_dark.isEmpty() && QFile::exists(filename_dark)) {
        path = filename_dark;
    }

    if (path.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        const SlideshowData sData = XmlFinder::parseSlideshowXml(path, m_requestedSize);

        if (sData.data.empty()) {
            Q_EMIT done(QImage());
            return;
        }

        const QDateTime startTime = XmlSlideshowUpdateTimer::slideshowStartTime(sData);
        const qint64 timeDiff = startTime.secsTo(QDateTime::currentDateTime());

        // Caculate cycle length
        TimeList timeList;
        qint64 totalTime;
        std::tie(timeList, totalTime) = XmlSlideshowUpdateTimer::slideshowTimeList(sData);

        if (totalTime > 0) {
            // Mod
            const qint64 modTime = timeDiff % totalTime;

            for (int i = 0; i < static_cast<int>(timeList.size()); i++) {
                const auto &nextP = timeList.at(i);

                if (nextP.second > modTime) {
                    const auto &p = timeList.at(i - 1);
                    const SlideshowItemData &item = sData.data.at(i - 1);

                    if (p.first == 0) {
                        // static
                        path = item.file;
                    } else {
                        // Blend two images, like gdk_pixbuf_composite
                        QImage from(item.from);
                        QImage to(item.to);
                        const double opacity = 1.0 - (nextP.second - modTime) / static_cast<double>(nextP.second - p.second);

                        blendImages(from, to, opacity);

                        if (!from.isNull() && m_requestedSize.isValid()) {
                            from = from.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        }

                        Q_EMIT done(from);
                        return;
                    }

                    break;
                }
            }
        }
    }

    QImage image(path);

    if (!image.isNull() && m_requestedSize.isValid()) {
        image = image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    Q_EMIT done(image);
}

void AsyncXmlImageResponseRunnable::blendImages(QImage &from, QImage &to, double toOpacity) const
{
    if (from.isNull() || toOpacity < 0 || toOpacity > 1) {
        return;
    }

    from = from.convertToFormat(QImage::Format_ARGB32);
    to = to.convertToFormat(QImage::Format_ARGB32);

    auto p = std::make_unique<QPainter>();

    if (!p->begin(&from)) {
        return;
    }

    p->setOpacity(toOpacity);
    p->drawImage(QRect(0, 0, from.width(), from.height()), to);
    p->end();
}

AsyncXmlImageResponse::AsyncXmlImageResponse(const QString &path, const QSize &requestedSize, QThreadPool *pool)
{
    auto runnable = new AsyncXmlImageResponseRunnable(path, requestedSize);
    connect(runnable, &AsyncXmlImageResponseRunnable::done, this, &AsyncXmlImageResponse::slotHandleDone);
    pool->start(runnable);
}

void AsyncXmlImageResponse::slotHandleDone(const QImage &image)
{
    m_image = image;
    emit finished();
}

QQuickTextureFactory *AsyncXmlImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

XmlImageProvider::XmlImageProvider()
{
}

QQuickImageResponse *XmlImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    AsyncXmlImageResponse *response = new AsyncXmlImageResponse(id, requestedSize, &m_pool);

    return response;
}

#include "xmlimageprovider.moc"
