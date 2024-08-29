/*
    SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "config-klipper.h"
#include "historyitem.h"
#include "mimetypes.h"
#include <KUrlMimeData>
#include <QCoreApplication>
#include <QDataStream>
#include <QFile>
#include <QImage>
#include <QMimeData>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <zlib.h>

using namespace Qt::StringLiterals;

enum class Result {
    Success,
    Failure,
    Skip,
};

using NullItem = std::monostate;
struct UrlItem {
    QList<QUrl> urls;
    QMap<QString, QString> metaData;
    int isCut;
};
using OldItemVariant = std::variant<NullItem, QString, QImage, UrlItem>;
using OldItemList = QList<OldItemVariant>;
using NewItemList = QList<HistoryItemSharedPtr>;

static const QString relativeHistoryFilePath = u"klipper/history2.lst"_s;

inline QString locateHistoryFile()
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, relativeHistoryFilePath);
}

// Based on the old version of HistoryItem::create(QDataStream &dataStream)
OldItemVariant createOldItem(QDataStream &dataStream)
{
    if (dataStream.atEnd()) {
        return NullItem{};
    }
    QString type;
    dataStream >> type;
    if (type == QLatin1String("url")) {
        UrlItem urlItem;
        dataStream >> urlItem.urls;
        dataStream >> urlItem.metaData;
        dataStream >> urlItem.isCut;
        return urlItem;
    }
    if (type == QLatin1String("string")) {
        QString text;
        dataStream >> text;
        return text;
    }
    if (type == QLatin1String("image")) {
        QImage image;
        dataStream >> image;
        return image;
    }
    qDebug() << "Failed to restore history item: Unknown type \"" << type << "\"";
    return NullItem{};
}

// Based on HistoryModel::loadHistory
Result loadHistory(OldItemList &oldItems)
{
    constexpr auto failedLoadWarning = "Failed to load history2.lst:";
    // don't use "appdata", klipper is also a kicker applet
    auto historyFilePath = locateHistoryFile();
    if (historyFilePath.isEmpty()) {
        return Result::Skip;
    }

    QFile historyFile(historyFilePath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        qDebug() << failedLoadWarning << historyFile.errorString();
        return Result::Failure;
    }

    QDataStream fileStream(&historyFile);
    if (fileStream.atEnd()) {
        qDebug() << failedLoadWarning << "Error in reading data";
        return Result::Failure;
    }

    QByteArray data;
    quint32 crc;
    fileStream >> crc >> data;
    if (crc32(0, reinterpret_cast<unsigned char *>(data.data()), data.size()) != crc) {
        qDebug() << failedLoadWarning << "CRC checksum does not match";
        return Result::Failure;
    }

    QDataStream historyStream(&data, QIODevice::ReadOnly);
    char *version;
    historyStream >> version;
    delete[] version;

    // The last row is items.size() - 1.
    for (auto item = createOldItem(historyStream); //
         !std::holds_alternative<NullItem>(item); //
         item = createOldItem(historyStream)) {
        oldItems.emplace_back(std::move(item));
    }

    return Result::Success;
}

// Based on HistoryModel::saveHistory
Result saveHistory(const NewItemList &newItems)
{
    constexpr auto failedSaveWarning = "Failed to save history2.lst:";
    // don't use "appdata", klipper is also a kicker applet
    const auto historyFilePath = locateHistoryFile();
    if (historyFilePath.isEmpty()) {
        return Result::Skip;
    }

    QSaveFile historyFile(historyFilePath);
    if (!historyFile.open(QIODevice::WriteOnly)) {
        qDebug() << failedSaveWarning << historyFile.errorString();
        return Result::Failure;
    }

    QByteArray data;
    QDataStream history_stream(&data, QIODevice::WriteOnly);
    history_stream << KLIPPER_VERSION_STRING; // const char*

    if (!newItems.empty()) {
        for (const auto &item : newItems) {
            history_stream << item.get();
        }
    }

    quint32 crc = crc32(0, reinterpret_cast<unsigned char *>(data.data()), data.size());
    QDataStream ds(&historyFile);
    ds << crc << data;
    if (!historyFile.commit()) {
        qDebug() << failedSaveWarning << "failed to commit updated save file to disk.";
        return Result::Failure;
    }

    return Result::Success;
}

/**
 * In Plasma 6.3, the save format for klipper history was changed to have each
 * entry represent a QVariantMap of MIME types and data.
 *
 * @since 6.3
 */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    OldItemList oldItems;
    if (auto result = loadHistory(oldItems); result != Result::Success) {
        return result == Result::Skip ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    NewItemList newItems;
    static const auto visitor = [](const auto &arg) -> HistoryItemPtr {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, QString>) {
            return HistoryItem::create(arg);
        } else if constexpr (std::is_same_v<T, QImage>) {
            return HistoryItem::create(arg);
        } else if constexpr (std::is_same_v<T, UrlItem>) {
            QMimeData mimeData;
            mimeData.setUrls(arg.urls);
            KUrlMimeData::setMetaData(arg.metaData, &mimeData);
            mimeData.setData(Mimetypes::Application::xKdeCutselection, arg.isCut ? "1" : "0");
            return HistoryItem::create(&mimeData);
        }
        return nullptr;
    };
    for (const auto &variant : std::as_const(oldItems)) {
        auto item = std::visit(visitor, variant);
        if (item == nullptr) {
            return EXIT_FAILURE; // This should not happen
        }
        newItems.emplace_back(std::move(item));
    }
    if (auto result = saveHistory(newItems); result != Result::Success) {
        return result == Result::Skip ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
