/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "historystringitem.h"

#include <QCryptographicHash>

HistoryStringItem::HistoryStringItem(const QString &data)
    : HistoryItem(QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha1))
    , m_data(data)
{
}

/* virtual */
void HistoryStringItem::write(QDataStream &stream) const
{
    stream << QStringLiteral("string") << m_data;
}

QMimeData *HistoryStringItem::mimeData() const
{
    QMimeData *data = new QMimeData();
    data->setText(m_data);
    return data;
}
