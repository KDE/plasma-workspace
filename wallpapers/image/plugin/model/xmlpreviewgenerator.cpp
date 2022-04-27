/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "xmlpreviewgenerator.h"

#include <cmath>

#include <QEventLoop>
#include <QFile>
#include <QPainter>

#include <KIO/PreviewJob>

XmlPreviewGenerator::XmlPreviewGenerator(const WallpaperItem &item, const QSize &size, QObject *parent)
    : QObject(parent)
    , m_item(item)
    , m_screenshotSize(size)
{
}

void XmlPreviewGenerator::run()
{
    if (!QFile::exists(m_item.filename)) {
        // At least the light wallpaper must be available
        Q_EMIT failed(m_item);
        return;
    }

    QPixmap preview;

    if (!m_item.slideshow.data.empty() || QFile::exists(m_item.filename_dark)) {
        // Slideshow preview
        preview = generateSlideshowPreview();
    } else {
        preview = generateSinglePreview();
    }

    if (preview.isNull()) {
        Q_EMIT failed(m_item);
        return;
    }

    Q_EMIT gotPreview(m_item, preview);
}

QPixmap XmlPreviewGenerator::generateSinglePreview()
{
    QEventLoop loop;
    QPixmap pixmap;

    const QUrl url = QUrl::fromLocalFile(m_item.filename);
    const QStringList availablePlugins = KIO::PreviewJob::availablePlugins();

    KIO::PreviewJob *const job = KIO::filePreview(KFileItemList{KFileItem(url, QString(), 0)}, m_screenshotSize, &availablePlugins);
    job->setIgnoreMaximumSize(true);

    loop.connect(job, &KIO::PreviewJob::gotPreview, this, [&](const KFileItem &item, const QPixmap &preview) {
        Q_UNUSED(item)
        pixmap = preview;
        loop.quit();
    });
    loop.connect(job, &KIO::PreviewJob::failed, &loop, &QEventLoop::quit);

    loop.exec();

    return pixmap;
}

QPixmap XmlPreviewGenerator::generateSlideshowPreview()
{
    int staticCount = 0;

    std::vector<QImage> list;
    list.reserve(std::max<int>(m_item.slideshow.data.size(), 2));

    if (!m_item.slideshow.data.empty()) {
        for (const auto &item : std::as_const(m_item.slideshow.data)) {
            if (item.dataType == 0) {
                const QImage image(item.file);

                if (image.isNull()) {
                    continue;
                }

                if (m_screenshotSize.width() > m_screenshotSize.height()) {
                    list.emplace_back(image.scaledToHeight(m_screenshotSize.height(), Qt::SmoothTransformation));
                } else {
                    list.emplace_back(image.scaledToWidth(m_screenshotSize.width(), Qt::SmoothTransformation));
                }

                staticCount += 1;
            }
        }
    } else {
        if (m_screenshotSize.width() > m_screenshotSize.height()) {
            list.emplace_back(QImage(m_item.filename).scaledToHeight(m_screenshotSize.height(), Qt::SmoothTransformation));
            list.emplace_back(QImage(m_item.filename_dark).scaledToHeight(m_screenshotSize.height(), Qt::SmoothTransformation));
        } else {
            list.emplace_back(QImage(m_item.filename).scaledToWidth(m_screenshotSize.width(), Qt::SmoothTransformation));
            list.emplace_back(QImage(m_item.filename_dark).scaledToWidth(m_screenshotSize.width(), Qt::SmoothTransformation));
        }

        staticCount = 2;
    }

    if (staticCount == 0) {
        return QPixmap();
    }

    QPixmap pix(list.at(0).size());
    pix.fill(Qt::transparent);
    auto p = std::make_unique<QPainter>();

    if (!p->begin(&pix)) {
        return pix;
    }

    for (int i = 0; i < static_cast<int>(list.size()); i++) {
        const QImage &image = list.at(i);

        double start = i / static_cast<double>(list.size()), end = (i + 1) / static_cast<double>(list.size());

        QPoint topLeft(std::lround(start * image.width()), 0);
        QPoint bottomRight(std::lround(end * image.width()), image.height());
        QPoint topLeft2(std::lround(start * pix.width()), 0);
        QPoint bottomRight2(std::lround(end * pix.width()), pix.height());

        p->drawImage(QRect(topLeft2, bottomRight2), image.copy(QRect(topLeft, bottomRight)));
    }

    p->end();

    return pix;
}
