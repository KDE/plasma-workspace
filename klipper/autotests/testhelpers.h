/*
    SPDX-FileCopyrightText: 2026 Tomáš Hnyk <tomashnyk@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QBuffer>
#include <QCryptographicHash>
#include <QImageReader>
#include <QString>

// UUID of an entry, mirrors HistoryModel implementation
inline QString uuidOf(const QString &text)
{
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex());
}

inline std::pair<QImage, QString> imageAndErrorForData(const QByteArray &data, const char *format = nullptr) {
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODeviceBase::ReadOnly);
    QImageReader imageReader(&buffer, format);
    return {imageReader.read(), imageReader.errorString()};
}
