/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "clipboardjob.h"
#include "klipper.h"
#include "history.h"
#include "historyitem.h"

#include <KIO/PreviewJob>
#include <QDebug>
#include <QIcon>

const static QString s_iconKey = QStringLiteral("icon");
const static QString s_previewKey = QStringLiteral("preview");
const static QString s_previewWidthKey = QStringLiteral("previewWidth");
const static QString s_previewHeightKey = QStringLiteral("previewHeight");
const static QString s_urlKey = QStringLiteral("url");

ClipboardJob::ClipboardJob(Klipper *klipper, const QString &destination, const QString &operation, const QVariantMap &parameters, QObject *parent)
    : Plasma::ServiceJob(destination, operation, parameters, parent)
    , m_klipper(klipper)
{
}

void ClipboardJob::start()
{
    const QString operation = operationName();
    // first check for operations not needing an item
    if (operation == QLatin1String("clearHistory")) {
        m_klipper->slotAskClearHistory();
        setResult(true);
        emitResult();
        return;
    } else if (operation == QLatin1String("configureKlipper")) {
        m_klipper->slotConfigure();
        setResult(true);
        emitResult();
        return;
    }

    // other operations need the item
    HistoryItemConstPtr item = m_klipper->history()->find(QByteArray::fromBase64(destination().toUtf8()));
    if (item.isNull()) {
        setResult(false);
        emitResult();
        return;
    }
    if (operation == QLatin1String("select")) {
        m_klipper->history()->slotMoveToTop(item->uuid());
        setResult(true);
    } else if (operation == QLatin1String("remove")) {
        m_klipper->history()->remove(item);
        setResult(true);
    } else if (operation == QLatin1String("edit")) {
        connect(m_klipper, &Klipper::editFinished, this,
            [this, item](HistoryItemConstPtr editedItem, int result) {
                if (item != editedItem) {
                    // not our item
                    return;
                }
                setResult(result);
                emitResult();
            }
        );
        m_klipper->editData(item);
        return;
    } else if (operation == QLatin1String("barcode")) {
#ifdef HAVE_PRISON
        m_klipper->showBarcode(item);
        setResult(true);
#else
        setResult(false);
#endif
    } else if (operation == QLatin1String("action")) {
        m_klipper->urlGrabber()->invokeAction(item);
        setResult(true);

    } else if (operation == s_previewKey) {
        const int pixelWidth = parameters().value(s_previewWidthKey).toInt();
        const int pixelHeight = parameters().value(s_previewHeightKey).toInt();
        QUrl url = parameters().value(s_urlKey).toUrl();
        qDebug() << "URL: " << url;
        KFileItem item(url);

        if (pixelWidth <= 0 || pixelHeight <= 0) {
            qWarning() << "Preview size invalid: " << pixelWidth << "x" << pixelHeight;
            iconResult(item);
            return;
        }

        if (!url.isValid() || !url.isLocalFile()) { // no remote files
            qWarning() << "Invalid or non-local url for preview: " << url;
            iconResult(item);
            return;
        }

        KFileItemList urls;
        urls << item;

        KIO::PreviewJob* job = KIO::filePreview(urls, QSize(pixelWidth, pixelHeight));
        job->setIgnoreMaximumSize(true);
        connect(job, &KIO::PreviewJob::gotPreview, this,
            [this](const KFileItem &item, const QPixmap &preview) {
                QVariantMap res;
                res.insert(s_urlKey, item.url());
                res.insert(s_previewKey, preview);
                res.insert(s_iconKey, false);
                res.insert(s_previewWidthKey, preview.size().width());
                res.insert(s_previewHeightKey, preview.size().height());
                setResult(res);
                emitResult();
            }
        );
        connect(job, &KIO::PreviewJob::failed, this,
            [this](const KFileItem &item) {
                iconResult(item);
            }
        );

        job->start();

        return;
    } else {
        setResult(false);
    }
    emitResult();
}

void ClipboardJob::iconResult(const KFileItem& item)
{
    QVariantMap res;
    res.insert(s_urlKey, item.url());
    QPixmap pix = QIcon::fromTheme(item.currentMimeType().iconName()).pixmap(128, 128);
    res.insert(s_previewKey, pix);
    res.insert(s_iconKey, true);
    res.insert("iconName", item.currentMimeType().iconName());
    res.insert(s_previewWidthKey, pix.size().width());
    res.insert(s_previewHeightKey, pix.size().height());
    setResult(res);
    emitResult();
}
