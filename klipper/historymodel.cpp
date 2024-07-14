/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historymodel.h"

#include <chrono>
#include <zlib.h>

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtConcurrentRun>

#include <KLocalizedString>
#include <KMessageBox>

#include "config-klipper.h"
#include "historyitem.h"
#include "klipper_debug.h"
#include "systemclipboard.h"

using namespace std::chrono_literals;

std::shared_ptr<HistoryModel> HistoryModel::self()
{
    static std::weak_ptr<HistoryModel> instance;
    if (instance.expired()) {
        struct make_shared_enabler : public HistoryModel {
        };
        std::shared_ptr<HistoryModel> ptr = std::make_shared<make_shared_enabler>();
        instance = ptr;
        return ptr;
    }
    return instance.lock();
}

HistoryModel::HistoryModel()
    : QAbstractListModel(nullptr)
    , m_maxSize(0)
    , m_displayImages(true)
{
    m_saveFileTimer.setSingleShot(true);
    connect(&m_saveFileTimer, &QTimer::timeout, this, [this] {
        const QFuture<bool> future = QtConcurrent::run(&HistoryModel::saveHistory, this, false);
        // Destroying the future neither waits nor cancels the asynchronous computation
    });
}

HistoryModel::~HistoryModel()
{
}

void HistoryModel::clear()
{
    QMutexLocker lock(&m_mutex);
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void HistoryModel::clearHistory()
{
    int clearHist = KMessageBox::warningContinueCancel(nullptr,
                                                       i18n("Do you really want to clear and delete the entire clipboard history?"),
                                                       i18n("Clear Clipboard History"),
                                                       KStandardGuiItem::del(),
                                                       KStandardGuiItem::cancel(),
                                                       QStringLiteral("klipperClearHistoryAskAgain"),
                                                       KMessageBox::Dangerous);
    if (clearHist == KMessageBox::Continue) {
        clear();
        startSaveHistoryTimer();
    }
}

void HistoryModel::setMaxSize(int size)
{
    if (m_maxSize == size) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    m_maxSize = size;
    if (m_items.count() > m_maxSize) {
        removeRows(m_maxSize, m_items.count() - m_maxSize);
    }
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.count();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count() || index.column() != 0) {
        return QVariant();
    }

    const std::shared_ptr<HistoryItem> &item = m_items.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item->text();
    case Qt::DecorationRole:
        return item->image();
    case HistoryItemConstPtrRole:
        return QVariant::fromValue<HistoryItemConstPtr>(std::const_pointer_cast<const HistoryItem>(item));
    case UuidRole:
        return item->uuid();
    case TypeRole:
        return QVariant::fromValue<HistoryItemType>(item->type());
    case Base64UuidRole:
        return item->uuid().toBase64();
    case TypeIntRole:
        return int(item->type());
    }
    return QVariant();
}

bool HistoryModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid()) {
        return false;
    }
    if ((row + count) > m_items.count()) {
        return false;
    }
    QMutexLocker lock(&m_mutex);
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_items.removeAt(row);
    }
    endRemoveRows();
    return true;
}

bool HistoryModel::remove(const QByteArray &uuid)
{
    const int index = indexOf(uuid);
    if (index < 0) {
        return false;
    }
    return removeRow(index, QModelIndex());
}

int HistoryModel::indexOf(const QByteArray &uuid) const
{
    auto it = std::find_if(m_items.cbegin(), m_items.cend(), [&uuid](const auto &item) {
        return item->uuid() == uuid;
    });
    return it == m_items.cend() ? -1 : std::distance(m_items.cbegin(), it);
}

int HistoryModel::indexOf(const HistoryItem *item) const
{
    if (!item) [[unlikely]] {
        return -1;
    }
    return indexOf(item->uuid());
}

void HistoryModel::insert(const std::shared_ptr<HistoryItem> &item)
{
    if (!item) {
        return;
    }

    if (m_maxSize == 0) {
        // special case - cannot insert any items
        return;
    }

    QMutexLocker lock(&m_mutex);

    if (const int existingItemIndex = indexOf(item.get()); existingItemIndex >= 0) {
        // move to top
        moveToTop(existingItemIndex);
        return;
    }

    beginInsertRows(QModelIndex(), 0, 0);
    item->setModel(this);
    m_items.prepend(item);
    endInsertRows();

    if (m_items.count() > m_maxSize) {
        beginRemoveRows(QModelIndex(), m_items.count() - 1, m_items.count() - 1);
        m_items.removeLast();
        endRemoveRows();
    }
}

bool HistoryModel::loadHistory()
{
    if (m_maxSize == 0) [[unlikely]] {
        // rare special case - cannot insert any items
        return true;
    }

    constexpr const char *failedLoadWarning = "Failed to load history resource. Clipboard history cannot be read.";
    // don't use "appdata", klipper is also a kicker applet
    QString historyFilePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("klipper/history2.lst"));
    if (historyFilePath.isEmpty()) {
        qCWarning(KLIPPER_LOG) << failedLoadWarning << ": "
                               << "History file does not exist";
        return false;
    }

    QFile historyFile(historyFilePath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        qCWarning(KLIPPER_LOG) << failedLoadWarning << ": " << historyFile.errorString();
        return false;
    }

    QDataStream fileStream(&historyFile);
    if (fileStream.atEnd()) {
        qCWarning(KLIPPER_LOG) << failedLoadWarning << ": "
                               << "Error in reading data";
        return false;
    }

    QByteArray data;
    quint32 crc;
    fileStream >> crc >> data;
    if (crc32(0, reinterpret_cast<unsigned char *>(data.data()), data.size()) != crc) {
        qCWarning(KLIPPER_LOG) << failedLoadWarning << ": "
                               << "CRC checksum does not match";
        return false;
    }

    QDataStream historyStream(&data, QIODevice::ReadOnly);
    char *version;
    historyStream >> version;
    delete[] version;

    // The last row is either items.size() - 1 or m_maxSize - 1.
    decltype(m_items) items;
    for (HistoryItemPtr item = HistoryItem::create(historyStream); item && items.size() < m_maxSize; item = HistoryItem::create(historyStream)) {
        item->setModel(this);
        items.emplace_back(std::move(item));
    }

    if (items.empty()) {
        // special case - nothing to insert, so just clear.
        clear();
        return true;
    }

    {
        QMutexLocker lock(&m_mutex);
        beginResetModel();
        m_items = std::move(items);
        endResetModel();
    }

    SystemClipboard::self()->setMimeData(m_items[0], SystemClipboard::Clipboard | SystemClipboard::Selection);

    return true;
}

void HistoryModel::startSaveHistoryTimer(std::chrono::seconds delay)
{
    m_saveFileTimer.start(delay);
}

bool HistoryModel::saveHistory(bool empty)
{
    QMutexLocker lock(&m_mutex);
    constexpr const char *failedSaveWarning = "Failed to save history. Clipboard history cannot be saved. Reason:";
    static const QString relativeHistoryFilePath = QStringLiteral("klipper/history2.lst");
    // don't use "appdata", klipper is also a kicker applet
    QString historyFilePath(QStandardPaths::locate(QStandardPaths::GenericDataLocation, relativeHistoryFilePath));
    if (historyFilePath.isEmpty()) {
        // try creating the file

        QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        if (path.isEmpty()) {
            qCWarning(KLIPPER_LOG) << failedSaveWarning << "cannot locate a standard data location to save the clipboard history.";
            return false;
        }

        QDir dir(path);
        if (!dir.mkpath(QStringLiteral("klipper"))) {
            qCWarning(KLIPPER_LOG) << failedSaveWarning << "Klipper save directory" << path + QStringLiteral("/klipper")
                                   << "does not exist and cannot be created.";
            return false;
        }
        historyFilePath = dir.absoluteFilePath(relativeHistoryFilePath);
    }
    if (historyFilePath.isEmpty()) {
        qCWarning(KLIPPER_LOG) << failedSaveWarning << "could not construct path to save clipboard history to.";
        return false;
    }

    QSaveFile historyFile(historyFilePath);
    if (!historyFile.open(QIODevice::WriteOnly)) {
        qCWarning(KLIPPER_LOG) << failedSaveWarning << "unable to open save file" << historyFilePath << ":" << historyFile.errorString();
        return false;
    }

    QByteArray data;
    QDataStream history_stream(&data, QIODevice::WriteOnly);
    history_stream << KLIPPER_VERSION_STRING; // const char*

    if (!empty && !m_items.isEmpty()) {
        HistoryItemPtr item = m_items[0];
        if (item) {
            do {
                history_stream << item.get();
                item = m_items[indexOf(item->next_uuid())];
            } while (item != m_items[0]);
        }
    }

    quint32 crc = crc32(0, reinterpret_cast<unsigned char *>(data.data()), data.size());
    QDataStream ds(&historyFile);
    ds << crc << data;
    if (!historyFile.commit()) {
        qCWarning(KLIPPER_LOG) << failedSaveWarning << "failed to commit updated save file to disk.";
        return false;
    }

    return true;
}

void HistoryModel::moveToTop(const QByteArray &uuid)
{
    const int existingItemIndex = indexOf(uuid);
    if (existingItemIndex < 0) {
        return;
    }
    moveToTop(existingItemIndex);
}

void HistoryModel::moveToTop(int row)
{
    if (row == 0 || row >= m_items.count()) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
}

void HistoryModel::moveTopToBack()
{
    if (m_items.count() < 2) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    beginMoveRows(QModelIndex(), 0, 0, QModelIndex(), m_items.count());
    auto item = m_items.takeFirst();
    m_items.append(item);
    endMoveRows();
}

void HistoryModel::moveBackToTop()
{
    moveToTop(m_items.count() - 1);
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    QHash<int, QByteArray> hash;
    hash.insert(Qt::DisplayRole, QByteArrayLiteral("DisplayRole"));
    hash.insert(Qt::DecorationRole, QByteArrayLiteral("DecorationRole"));
    hash.insert(Base64UuidRole, QByteArrayLiteral("UuidRole"));
    hash.insert(TypeIntRole, QByteArrayLiteral("TypeRole"));
    return hash;
}
