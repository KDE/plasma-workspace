/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

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
#include "historystringitem.h"
#include "klipper_debug.h"
#include "klippersettings.h"
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
    , m_clip(SystemClipboard::self())
    , m_displayImages(true)
{
    m_saveFileTimer.setSingleShot(true);
    connect(&m_saveFileTimer, &QTimer::timeout, this, [this] {
        const QFuture<bool> future = QtConcurrent::run(&HistoryModel::saveHistory, this, false);
        // Destroying the future neither waits nor cancels the asynchronous computation
    });

    loadSettings();
    loadHistory();
    // Only connect to this signal after loading the history, to avoid the action of loading triggering a save
    auto modelChanged = [this](const QModelIndex & /*parent*/, int first, int /*last*/) {
        changed(first == 0);
    };
    connect(this, &HistoryModel::rowsInserted, this, modelChanged);
    connect(this, &HistoryModel::rowsRemoved, this, modelChanged);
    connect(this,
            &HistoryModel::rowsMoved,
            this,
            [this](const QModelIndex & /*sourceParent*/, int sourceStart, int /*sourceEnd*/, const QModelIndex & /*destinationParent*/, int destinationRow) {
                Q_EMIT changed(sourceStart == 0 || destinationRow == 0);
            });
    connect(this, &HistoryModel::modelReset, this, [this] {
        Q_EMIT changed(true);
    });

    connect(this, &HistoryModel::changed, this, [this](bool isTop) {
        if (m_items.empty()) {
            m_clip->clear(SystemClipboard::SelectionMode(SystemClipboard::Selection | SystemClipboard::Clipboard));
        }
        startSaveHistoryTimer();
        if (!isTop || m_items.empty() || m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
            return;
        }
        m_clip->setMimeData(m_items[0], SystemClipboard::SelectionMode(SystemClipboard::Clipboard | SystemClipboard::Selection));
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

qsizetype HistoryModel::maxSize() const
{
    return m_maxSize;
}

void HistoryModel::setMaxSize(qsizetype size)
{
    if (m_maxSize == size) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    m_maxSize = size;
    if (m_items.size() > m_maxSize) {
        removeRows(m_maxSize, m_items.size() - m_maxSize);
    }
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size() || index.column() != 0) {
        return QVariant();
    }

    const std::shared_ptr<HistoryItem> &item = m_items.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item->text();
    case Qt::DecorationRole: {
        return item->image();
    }
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

bool HistoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return false;
    }

    switch (auto &item = m_items[index.row()]; role) {
    case Qt::DisplayRole: {
        if (item->type() == HistoryItemType::Text && value.canConvert<QString>()) {
            item = std::make_shared<HistoryStringItem>(value.toString());
            Q_EMIT dataChanged(index, index, {Qt::DisplayRole});
            return true;
        }
        break;
    }
    }

    return false;
}

bool HistoryModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid()) {
        return false;
    }
    if (qsizetype(row + count) > m_items.size()) {
        return false;
    }
    QMutexLocker lock(&m_mutex);
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    m_items.erase(std::next(m_items.cbegin(), row), std::next(m_items.cbegin(), row + count));
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

HistoryItemConstPtr HistoryModel::first() const
{
    if (m_items.empty()) {
        return HistoryItemConstPtr();
    }
    return m_items[0];
}

void HistoryModel::insert(const std::shared_ptr<HistoryItem> &item)
{
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
    m_items.prepend(item);
    endInsertRows();

    if (m_items.size() > m_maxSize) {
        beginRemoveRows(QModelIndex(), m_items.size() - 1, m_items.size() - 1);
        m_items.pop_back();
        endRemoveRows();
    }
}

bool HistoryModel::loadHistory()
{
    if (m_maxSize == 0 || !KlipperSettings::keepClipboardContents()) [[unlikely]] {
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

    m_clip->setMimeData(m_items[0], SystemClipboard::SelectionMode(SystemClipboard::Clipboard | SystemClipboard::Selection));

    return true;
}

void HistoryModel::loadSettings()
{
    setMaxSize(KlipperSettings::maxClipItems());
    m_displayImages = !KlipperSettings::ignoreImages();
}

void HistoryModel::startSaveHistoryTimer(std::chrono::seconds delay)
{
    m_saveFileTimer.start(delay);
}

bool HistoryModel::saveHistory(bool empty)
{
    if (!KlipperSettings::keepClipboardContents()) [[unlikely]] {
        return true;
    }

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

    if (!empty && !m_items.empty()) {
        for (const auto &item : std::as_const(m_items)) {
            history_stream << item.get();
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

void HistoryModel::moveToTop(qsizetype row)
{
    if (row >= m_items.size()) [[unlikely]] {
        Q_ASSERT_X(false, Q_FUNC_INFO, std::to_string(row).c_str());
        return;
    }
    if (row == 0) {
        // The item is already at the top, but it still may be not be set as the actual clipboard
        // contents, normally this happens if the item is only in the X11 mouse selection but
        // not in the Ctrl+V clipboard.
        return;
    }
    QMutexLocker lock(&m_mutex);
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
}

void HistoryModel::moveTopToBack()
{
    if (m_items.size() < 2) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    beginMoveRows(QModelIndex(), 0, 0, QModelIndex(), m_items.size());
    auto item = m_items.takeFirst();
    m_items.append(item);
    endMoveRows();
}

void HistoryModel::moveBackToTop()
{
    moveToTop(m_items.size() - 1);
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    QHash<int, QByteArray> hash;
    hash.insert(Qt::DisplayRole, QByteArrayLiteral("display"));
    hash.insert(Qt::DecorationRole, QByteArrayLiteral("decoration"));
    hash.insert(UuidRole, QByteArrayLiteral("uuid"));
    hash.insert(TypeIntRole, QByteArrayLiteral("type"));
    return hash;
}
