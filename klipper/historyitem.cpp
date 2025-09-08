/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "historyitem.h"

#include <QMimeData>
#include <QSqlQuery>
#include <algorithm>

#include "historymodel.h"

using namespace Qt::StringLiterals;

HistoryItem::HistoryItem(const QString &uuid, const QStringList &mimeTypes, const QString &text)
    : m_uuid(uuid)
    , m_text(text)
{
    if (std::ranges::any_of(mimeTypes, [](const QString &mimeType) {
            return mimeType.startsWith(u"text/");
        })) {
        m_types |= HistoryItemType::Text;
    }
    if (mimeTypes.contains(u"text/uri-list"_s)) {
        m_types |= HistoryItemType::Url;
    }
    if (std::ranges::any_of(mimeTypes, [](const QString &mimeType) {
            return mimeType.startsWith(u"image/") || mimeType == u"application/x-qt-image";
        })) {
        m_types |= HistoryItemType::Image;
    }
}

HistoryItem::~HistoryItem() = default;

HistoryItemType HistoryItem::type() const
{
    if (m_types & HistoryItemType::Url) {
        return HistoryItemType::Url;
    } else if ((m_types & HistoryItemType::Text) && !m_text.isEmpty()) {
        return HistoryItemType::Text;
    } else if (m_types & HistoryItemType::Image) {
        return HistoryItemType::Image;
    }
    return HistoryItemType::Text;
}

QString HistoryItem::text() const
{
    return m_text;
}

HistoryItemPtr HistoryItem::create(const QSqlQuery &query)
{
    QString uuid = query.value(u"uuid"_s).toString();
    if (uuid.isEmpty()) {
        return HistoryItemPtr();
    }
    QStringList mimeTypes = query.value(u"mimetypes"_s).toString().split(u',');
    if (mimeTypes.empty()) {
        return HistoryItemPtr();
    }
    QString text = query.value(u"text"_s).toString();

    return std::make_shared<HistoryItem>(std::move(uuid), std::move(mimeTypes), std::move(text));
}

#include "moc_historyitem.cpp"
