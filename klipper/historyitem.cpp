/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "historyitem.h"

#include <QMimeData>
#include <QSqlQuery>

#include <kurlmimedata.h>

#include "historymodel.h"

using namespace Qt::StringLiterals;

HistoryItem::HistoryItem(QString &&uuid, QStringList &&mimeTypes, QString &&text)
    : m_uuid(std::move(uuid))
    , m_text(std::move(text))
{
    if (std::any_of(mimeTypes.cbegin(), mimeTypes.cend(), [](const QString &mimeType) {
            return mimeType.startsWith(u"text/");
        })) {
        m_types |= HistoryItemType::Text;
    }
    if (mimeTypes.contains(u"text/uri-list"_s)) {
        m_types |= HistoryItemType::Url;
    }
    if (std::any_of(mimeTypes.cbegin(), mimeTypes.cend(), [](const QString &mimeType) {
            return mimeType.startsWith(u"image/") || mimeType == u"application/x-qt-image";
        })) {
        m_types |= HistoryItemType::Image;
    }
}

HistoryItem::~HistoryItem()
{
}

HistoryItemType HistoryItem::type() const
{
    if (m_types & HistoryItemType::Url) {
        return HistoryItemType::Url;
    } else if (m_types & HistoryItemType::Text) {
        return HistoryItemType::Text;
    } else if (m_types & HistoryItemType::Image) {
        return HistoryItemType::Image;
    }
    return HistoryItemType::Unknown;
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
