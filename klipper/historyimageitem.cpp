/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historyimageitem.h"

#include "historymodel.h"

#include <QCryptographicHash>
#include <QIODevice>
#include <QIcon>
#include <QMimeData>

#include <KLocalizedString>

namespace
{
QByteArray compute_uuid(const QPixmap &data)
{
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << data;
    return QCryptographicHash::hash(buffer, QCryptographicHash::Sha1);
}

}

HistoryImageItem::HistoryImageItem(const QPixmap &data)
    : HistoryItem(compute_uuid(data))
    , m_data(data)
{
}

QString HistoryImageItem::text() const
{
    if (m_text.isNull()) {
        m_text = QStringLiteral("▨ ") + i18n("%1x%2 %3bpp", m_data.width(), m_data.height(), m_data.depth());
    }
    return m_text;
}

/* virtual */
void HistoryImageItem::write(QDataStream &stream) const
{
    stream << QStringLiteral("image") << m_data;
}

QMimeData *HistoryImageItem::mimeData() const
{
    QMimeData *data = new QMimeData();
    data->setImageData(m_data.toImage());
    return data;
}

const QPixmap &HistoryImageItem::image() const
{
    if (m_model->displayImages()) {
        return m_data;
    }
    static QPixmap imageIcon(QIcon::fromTheme(QStringLiteral("view-preview")).pixmap(QSize(48, 48)));
    return imageIcon;
}
