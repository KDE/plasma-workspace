/* This file is part of the KDE project
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "historyimageitem.h"

#include "historymodel.h"

#include <QCryptographicHash>
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
