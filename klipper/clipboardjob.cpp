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

ClipboardJob::ClipboardJob(Klipper *klipper, const QString &destination, const QString &operation, const QVariantMap &parameters, QObject *parent)
    : Plasma::ServiceJob(destination, operation, parameters, parent)
    , m_klipper(klipper)
{
}

void ClipboardJob::start()
{
    const QString operation = operationName();
    qDebug()<< "OP: " << operation << parameters();
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

    } else if (operation == QStringLiteral("preview")) {

            const int pixelWidth = parameters().value("previewWidth").toInt();
            const int pixelHeight = parameters().value("previewHeight").toInt();

            if (pixelWidth <= 0 || pixelHeight <= 0) {
                qWarning() << "Preview size invalid: " << pixelWidth << "x" << pixelHeight;
                setResult(false);
                emitResult();
                return;
            }

            QUrl url(parameters().value("url").toString());
            if (!url.isValid() || !url.isLocalFile()) { // no remote files
                setResult(false);
                emitResult();
                return;
            }

            KFileItemList urls;
            urls << url;
            KIO::PreviewJob* job = KIO::filePreview(urls,
                                                    QSize(pixelWidth, pixelHeight));
            connect(job, &KIO::PreviewJob::gotPreview, this,
                [this](KFileItem item, QPixmap preview) {
                    qDebug() << "============== Preview arrived: " << item.url() << preview.size();
                    QVariantMap res;
                    res.insert("url", item.url());
                    res.insert("preview", preview);
                    setResult(res);
                    emitResult();
                }
            );
            connect(job, &KIO::PreviewJob::failed, this,
                [this](KFileItem item) {

                    qWarning() << "PreviewJob failed for" << item.url() << qobject_cast<KIO::PreviewJob*>(sender())->errorString();
                    setResult(false);
                    emitResult();
                }
            );


            job->start();

            return;
    } else {
        setResult(false);
    }
    emitResult();
}
