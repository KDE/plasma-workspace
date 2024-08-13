/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historyimageitem.h"

#include "historymodel.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QIcon>
#include <QMimeData>

#include <KLocalizedString>

namespace
{
QByteArray compute_uuid(const QImage &data)
{
    return QCryptographicHash::hash(QByteArray::fromRawData(reinterpret_cast<const char *>(data.constBits()), data.sizeInBytes()), QCryptographicHash::Sha1);
}

}

HistoryImageItem::HistoryImageItem(const QImage &image, const QString &mimetype, const QByteArray &data)
    : HistoryItem(compute_uuid(image))
    , m_image(image)
    , m_mimetype(mimetype)
    , m_data(data)
{
}

QString HistoryImageItem::text() const
{
    if (m_text.isNull()) {
        m_text = u"▨ " + i18n("%1x%2 %3bpp", m_image.width(), m_image.height(), m_image.depth());
    }
    return m_text;
}

/* virtual */
void HistoryImageItem::write(QDataStream &stream) const
{
    stream << QStringLiteral("image") << m_image;
}

QMimeData *HistoryImageItem::mimeData() const
{
    QMimeData *data = new QMimeData();
    if (!m_mimetype.isEmpty() && !m_data.isEmpty()) {
        data->setData(m_mimetype, m_data);
        // Also give PNG for compatibility.
        static const auto imagePng = QStringLiteral("image/png");
        if (m_mimetype != imagePng) {
            // QBuffer buffer;
            // buffer.open(QBuffer::ReadWrite);
            // m_image.save(&buffer, "png");
            // buffer.reset();
            // data->setData(imagePng, buffer.readAll());
        }
    } else {
        data->setImageData(m_image);
    }
    return data;
}

QImage HistoryImageItem::image() const
{
    return m_image;
}
