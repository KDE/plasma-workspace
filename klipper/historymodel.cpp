/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historymodel.h"

#include <algorithm>
#include <chrono>
#include <zlib.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QMimeData>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include <KIO/DeleteJob>
#include <KLocalizedString>
#include <KMessageBox>

#include "config-klipper.h"
#include "historyitem.h"
#include "klipper_debug.h"
#include "klippersettings.h"
#include "systemclipboard.h"
#include "updateclipboardjob.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

namespace
{
struct TransactionGuard {
    TransactionGuard(QSqlDatabase *db)
        : db(db)
        , inited(db->transaction())
    {
        if (!inited) {
            qCWarning(KLIPPER_LOG) << "A transaction didn't start:" << db->lastError().text();
        }
    }

    ~TransactionGuard()
    {
        if (!inited) {
            return;
        }
        if (!committed) {
            qCWarning(KLIPPER_LOG) << "Transaction failed:" << db->lastError().text();
            db->rollback();
        }
        db->commit();
    }

    bool exec(QSqlQuery &query)
    {
        if (!inited || !committed) {
            return false;
        }
        committed = query.exec();
        if (!committed) {
            qCWarning(KLIPPER_LOG).nospace().noquote() << "Query \"" << query.lastQuery() << "\" failed: " << query.lastError().text();
        }
        return committed;
    }

    bool exec(const QString &query)
    {
        QSqlQuery sqlQuery(query, *db);
        return exec(sqlQuery);
    }

    QSqlDatabase *db;
    bool inited = false;
    bool committed = true;
};

QString computeUuid(const QMimeData *data)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (data->hasText()) {
        hash.addData(data->text().toUtf8());
    }
    if (data->hasUrls()) {
        for (const auto urls = data->urls(); const QUrl &url : urls) {
            hash.addData(url.toEncoded());
        }
    }
    if (data->hasImage()) {
        const auto image = data->imageData().value<QImage>();
        hash.addData(QByteArrayView(reinterpret_cast<const char *>(image.constBits()), image.sizeInBytes()));
    }
    return QString::fromLatin1(hash.result().toHex());
}
}

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
    if (!QSqlDatabase::isDriverAvailable(u"QSQLITE"_s)) {
        qCCritical(KLIPPER_LOG) << "SQLITE driver isn't available";
        return;
    }

    loadSettings();
    if (!loadHistory()) [[unlikely]] {
        return;
    }

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
    connect(this, &HistoryModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &, const QList<int> &roles) {
        if (roles.contains(UuidRole)) { // Avoid overriding the current clipboard with async-loaded image data
            Q_EMIT changed(topLeft.row() == 0); // BUG 494031 the first item does not trigger rowsMoved
        }
    });
    connect(this, &HistoryModel::modelReset, this, [this] {
        Q_EMIT changed(true);
    });

    connect(this, &HistoryModel::changed, this, [this](bool isTop) {
        if (m_items.empty()) {
            m_clip->clear(SystemClipboard::SelectionMode(SystemClipboard::Selection | SystemClipboard::Clipboard));
            return;
        }
        if (!isTop) {
            return;
        }

        QSqlQuery query(m_db);
        query.prepare(u"UPDATE main SET last_used_time=? WHERE uuid='%1'"_s.arg(m_items[0]->uuid()));
        query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000.0);
        if (query.exec()) {
            if (m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
                return;
            }
            m_clip->setMimeData(m_items[0], SystemClipboard::SelectionMode(SystemClipboard::Clipboard | SystemClipboard::Selection));
        }
    });

    connect(m_clip.get(), &SystemClipboard::ignored, this, &HistoryModel::slotIgnored);
    connect(m_clip.get(), &SystemClipboard::newClipData, this, &HistoryModel::checkClipData);
}

HistoryModel::~HistoryModel()
{
    if (!m_bKeepContents) {
        m_db.close();
        QFile(m_db.databaseName()).remove();
        QDir(m_dbFolder + u"/data").removeRecursively();
    }
}

void HistoryModel::clear()
{
    if (!m_db.isOpen()) {
        return;
    }
    if (TransactionGuard transaction(&m_db); !transaction.exec(u"DELETE FROM main"_s) || !transaction.exec(u"DELETE FROM aux"_s)) {
        return;
    }
    QList<QUrl> deletedDataFolders;
    deletedDataFolders.reserve(m_items.size());
    std::ranges::transform(m_items, std::back_inserter(deletedDataFolders), [this](const auto &item) {
        return QUrl::fromLocalFile(m_dbFolder + u"/data/" + item->uuid() + u'/');
    });
    auto job = KIO::del(deletedDataFolders, KIO::HideProgressInfo);
    ++m_pendingJobs;
    connect(job, &KJob::finished, this, [this] {
        --m_pendingJobs;
    });
    QSqlQuery(u"VACUUM"_s, m_db).exec();
    if (!m_items.empty()) { // Is empty when m_bKeepContents is false
        beginResetModel();
        m_items.clear();
        m_starredCount = 0;
        endResetModel();
    }
    m_clip->clear(SystemClipboard::SelectionMode(SystemClipboard::Selection | SystemClipboard::Clipboard));
}

void HistoryModel::clearNonStarredHistory()
{
    if (!m_db.isOpen()) {
        return;
    }

    // Get UUIDs of all non-starred items
    QStringList nonStarredUuids;
    QSqlQuery query(m_db);
    query.exec(u"SELECT uuid FROM main WHERE starred = 0 OR starred IS NULL"_s);
    while (query.next()) {
        nonStarredUuids.append(query.value(0).toString());
    }

    if (nonStarredUuids.isEmpty()) {
        return; // No non-starred items to remove
    }

    // Delete non-starred items from database
    {
        TransactionGuard transaction(&m_db);
        QStringList quotedUuids;
        for (const QString &uuid : std::as_const(nonStarredUuids)) {
            quotedUuids.append(u'\'' + uuid + u'\'');
        }
        const QString uuidList = quotedUuids.join(u',');

        if (!transaction.exec(u"DELETE FROM main WHERE uuid IN (%1)"_s.arg(uuidList))) {
            return;
        }
        if (!transaction.exec(u"DELETE FROM aux WHERE uuid IN (%1)"_s.arg(uuidList))) {
            return;
        }
    }

    // Delete associated data folders
    QList<QUrl> deletedDataFolders;
    deletedDataFolders.reserve(nonStarredUuids.size());
    for (const QString &uuid : std::as_const(nonStarredUuids)) {
        deletedDataFolders.append(QUrl::fromLocalFile(m_dbFolder + u"/data/" + uuid + u'/'));
    }
    auto job = KIO::del(deletedDataFolders, KIO::HideProgressInfo);
    ++m_pendingJobs;
    connect(job, &KJob::finished, this, [this] {
        --m_pendingJobs;
    });

    // Remove items from the model that were deleted
    // We need to work backwards to maintain correct indices
    for (qsizetype i = m_items.size() - 1; i >= 0; --i) {
        if (nonStarredUuids.contains(m_items[i]->uuid())) {
            beginRemoveRows(QModelIndex(), i, i);
            m_items.removeAt(i);
            endRemoveRows();
        }
    }

    QSqlQuery(u"VACUUM"_s, m_db).exec();
    m_clip->clear(SystemClipboard::SelectionMode(SystemClipboard::Selection | SystemClipboard::Clipboard));
}

void HistoryModel::clearHistory()
{
    // First check if there are any starred items
    if (m_starredCount > 0) {
        // Offer choice to keep starred items or clear everything
        // TODO: Consider adding a setting in Klipper configuration UI to customize this behavior
        // (e.g., always ask, always keep starred, always clear all)
        int clearChoice = KMessageBox::questionTwoActionsCancel(nullptr,
                                                               i18n("The clipboard history contains starred items. Clear them too?"),
                                                               i18n("Clear Clipboard History"),
                                                               KGuiItem(i18n("Clear Everything"), QStringLiteral("edit-clear-history")),
                                                               KGuiItem(i18n("Keep Starred Items"), QStringLiteral("starred-symbolic")),
                                                               KStandardGuiItem::cancel(),
                                                               QStringLiteral("klipperClearHistoryStarredChoice"));

        if (clearChoice == KMessageBox::Cancel) {
            return; // User cancelled
        } else if (clearChoice == KMessageBox::ButtonCode::SecondaryAction) {
            // Keep starred items - delete only non-starred
            clearNonStarredHistory();
            return;
        }
        // If FirstAction (Clear All), fall through to normal clear()
    } else {
        // No starred items, show normal confirmation
        // TODO: Consider adding a "Reset 'Don't ask again' dialogs" button in Klipper settings
        // to help users recover from accidentally dismissed confirmations
        int clearHist = KMessageBox::warningContinueCancel(nullptr,
                                                           i18n("Do you really want to clear and delete the entire clipboard history?"),
                                                           i18n("Clear Clipboard History"),
                                                           KStandardGuiItem::del(),
                                                           KStandardGuiItem::cancel(),
                                                           QStringLiteral("klipperClearHistoryAskAgain"),
                                                           KMessageBox::Dangerous);
        if (clearHist != KMessageBox::Continue) {
            return; // User cancelled
        }
    }

    // Clear everything (including starred items)
    clear();
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
    m_maxSize = size;
    if (m_items.size() > m_maxSize) {
        // Remove non-starred items from the end until we're within the size limit
        // or until no more non-starred items are available
        // TODO: Consider adding a separate maxStarredItems setting in the future
        // to prevent starred items from causing unlimited history growth
        qsizetype itemsToRemove = m_items.size() - m_maxSize;
        qsizetype removedCount = 0;

        for (qsizetype i = m_items.size() - 1; i >= 0 && removedCount < itemsToRemove; --i) {
            // Check if item is starred
            QSqlQuery query(m_db);
            query.prepare(u"SELECT starred FROM main WHERE uuid = ?"_s);
            query.addBindValue(m_items[i]->uuid());
            bool isStarred = false;
            if (query.exec() && query.isSelect() && query.next()) {
                isStarred = query.value(0).toBool();
            }

            if (!isStarred) {
                removeRow(i);
                removedCount++;
                // Note: After removeRow, indices shift, but since we're going backwards
                // and only removing from the current position, this is safe
            }
        }
        // If we couldn't remove enough non-starred items, the history may exceed maxSize
        // This is acceptable as starred items are protected
    }
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QBindable<int> HistoryModel::bindableStarredCount() const
{
    return &m_starredCount;
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size() || index.column() != 0) {
        return {};
    }

    Q_ASSERT_X(m_db.isOpen(), Q_FUNC_INFO, qPrintable(m_db.lastError().text()));
    const std::shared_ptr<HistoryItem> &item = m_items.at(index.row());

    switch (role) {
    case Qt::DisplayRole: {
        if (item->type() == HistoryItemType::Image) {
            QSqlQuery query(u"SELECT data_uuid FROM aux WHERE uuid='%1' AND (mimetype='%2')"_s.arg(item->uuid(), s_imageFormat), m_db);
            if (query.exec() && query.isSelect() && query.next()) {
                QSize size = QImageReader(QString(m_dbFolder + u"/data/" + item->uuid() + u'/' + query.value(0).toString())).size();
                return QString(u"▨ " + i18nc("@info:tooltip width x height", "%1x%2", QString::number(size.width()), QString::number(size.height())));
            }
        }
        return item->text();
    }
    case ImageUrlRole: {
        if (!(item->allTypes() & HistoryItemType::Image)) {
            return QUrl();
        }
        QSqlQuery query(u"SELECT data_uuid FROM aux WHERE uuid='%1' AND (mimetype='%2')"_s.arg(item->uuid(), s_imageFormat), m_db);
        if (query.exec() && query.isSelect() && query.next()) {
            return QUrl::fromLocalFile(m_dbFolder + u"/data/" + item->uuid() + u'/' + query.value(0).toString());
        }
        return QUrl();
    }
    case ImageSizeRole: {
        if (!(item->allTypes() & HistoryItemType::Image)) {
            return QSize();
        }
        QSqlQuery query(u"SELECT data_uuid FROM aux WHERE uuid='%1' AND (mimetype='%2')"_s.arg(item->uuid(), s_imageFormat), m_db);
        if (query.exec() && query.isSelect() && query.next()) {
            return QImageReader(m_dbFolder + u"/data/" + item->uuid() + u'/' + query.value(0).toString()).size();
        }
        return QSize();
    }
    case HistoryItemConstPtrRole:
        return QVariant::fromValue<HistoryItemConstPtr>(std::const_pointer_cast<const HistoryItem>(item));
    case UuidRole:
        return item->uuid();
    case TypeRole:
        return QVariant::fromValue<HistoryItemType>(item->type());
    case TypeIntRole:
        return int(item->type());
    case StarredRole:
        // TODO: Consider adding QHash<QString, bool> cache for starred status to avoid
        // frequent database queries if performance becomes an issue with large histories
        return isItemStarred(item->uuid());
    }
    return {};
}

bool HistoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_ASSERT_X(m_db.isOpen(), Q_FUNC_INFO, qPrintable(m_db.lastError().text()));
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return false;
    }

    switch (auto &item = m_items[index.row()]; role) {
    case Qt::DisplayRole: {
        if (item->type() != HistoryItemType::Text || !value.canConvert<QString>()) [[unlikely]] {
            return false;
        }
        QString text = value.toString();
        if (text == item->text()) {
            return false;
        }
        QStringList mimetypes{u"text/plain"_s, u"text/plain;charset=utf-8"_s};
        QString newUuid = QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex());
        {
            TransactionGuard transaction(&m_db);
            {
                QSqlQuery query(m_db);
                query.prepare(u"UPDATE main SET (uuid, mimetypes, text)=(?, ?, ?) WHERE uuid='%1'"_s.arg(item->uuid()));
                query.addBindValue(newUuid);
                query.addBindValue(mimetypes.join(u','));
                query.addBindValue(text);
                // last_used_time is updated in the signal slot
                if (!transaction.exec(query) || !transaction.exec(u"DELETE FROM aux WHERE uuid='%1'"_s.arg(item->uuid()))) {
                    return false;
                }
            }

            for (const QString &mimetype : std::as_const(mimetypes)) {
                QSqlQuery query(m_db);
                query.prepare(u"INSERT INTO aux (uuid, mimetype, data_uuid) VALUES (?, ?, ?)"_s);
                query.addBindValue(newUuid);
                query.addBindValue(mimetype);
                query.addBindValue(newUuid);
                if (!transaction.exec(query)) {
                    return false;
                }
            }
        }

        KIO::del(QUrl::fromLocalFile(m_dbFolder + u"/data/" + item->uuid() + u'/'), KIO::HideProgressInfo);
        saveToFile(m_dbFolder, text.toUtf8(), newUuid, newUuid); // Must be synchronous so the clipboard can be updated immediately

        item = std::make_shared<HistoryItem>(std::move(newUuid), std::move(mimetypes), std::move(text));
        Q_EMIT dataChanged(index, index, {Qt::DisplayRole, UuidRole});
        return true;
    }
    case StarredRole: {
        bool newValue = value.toBool();
        if (newValue == isItemStarred(item->uuid())) {
            return false;
        }

        QSqlQuery query(m_db);
        // Use prepared statement for safety
        query.prepare(u"UPDATE main SET starred = ? WHERE uuid = ?"_s);
        query.addBindValue(newValue);
        query.addBindValue(item->uuid());

        if (query.exec()) {
            // Notify views that this specific role has changed for the item
            m_starredCount = m_starredCount + (newValue ? 1 : -1);
            Q_EMIT dataChanged(index, index, {StarredRole});
            return true;
        }
        // TODO: Add user feedback/notification when starring/unstarring fails
        // Currently users have no indication if the star operation was unsuccessful
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

    auto first = std::next(m_items.cbegin(), row);
    auto last = std::next(m_items.cbegin(), row + count);
    const int newStarredCount = m_starredCount - std::accumulate(first, last, 0, [this](int count, const std::shared_ptr<HistoryItem> &item) {
                                    return isItemStarred(item->uuid()) ? (count + 1) : count;
                                });

    {
        TransactionGuard transaction(&m_db);
        QStringList uuidList;
        for (int i = 0; i < count; ++i) {
            uuidList.append(u'\'' + m_items[row + i]->uuid() + u'\'');
        }
        const QString uuids = uuidList.join(u',');
        if (!transaction.exec(u"DELETE FROM main WHERE uuid IN (%1)"_s.arg(uuids))) {
            return false;
        }
        if (!transaction.exec(u"DELETE FROM aux WHERE uuid IN (%1)"_s.arg(uuids))) {
            return false;
        }
    }

    QList<QUrl> deletedDataFolders;
    deletedDataFolders.reserve(count);
    for (int i = 0; i < count; ++i) {
        deletedDataFolders.append(QUrl::fromLocalFile(m_dbFolder + u"/data/" + m_items[row + i]->uuid() + u'/'));
    }
    auto job = KIO::del(deletedDataFolders, KIO::HideProgressInfo);
    ++m_pendingJobs;
    connect(job, &KJob::finished, this, [this] {
        --m_pendingJobs;
    });

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    m_items.erase(first, last);
    m_starredCount = newStarredCount;
    endRemoveRows();
    return true;
}

bool HistoryModel::remove(const QString &uuid)
{
    const int index = indexOf(uuid);
    if (index < 0) {
        return false;
    }

    // Check if the item is starred before removing
    bool isStarred = isItemStarred(uuid);

    // Show confirmation dialog for starred items
    if (isStarred) {
        int result = KMessageBox::warningContinueCancel(nullptr,
                                                       i18n("This item is starred. Do you really want to remove it from history?"),
                                                       i18n("Remove Starred Item"),
                                                       KStandardGuiItem::del(),
                                                       KStandardGuiItem::cancel(),
                                                       QStringLiteral("klipperRemoveStarredItemAskAgain"));
        if (result != KMessageBox::Continue) {
            return false; // User cancelled deletion
        }
    }

    return removeRow(index, QModelIndex());
}

int HistoryModel::indexOf(const QString &uuid) const
{
    auto it = std::ranges::find_if(m_items, [&uuid](const auto &item) {
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
        return {};
    }
    return m_items[0];
}

bool HistoryModel::insert(const QMimeData *mimeData, qreal timestamp)
{
    if (m_maxSize == 0) {
        // special case - cannot insert any items
        return false;
    }
    QString uuid = computeUuid(mimeData);
    if (uuid.size() != 40 /*SHA1*/) [[unlikely]] {
        return false;
    }
    if (const int existingItemIndex = indexOf(uuid); existingItemIndex >= 0) {
        // move to top
        moveToTop(existingItemIndex);
        return true;
    }

    QStringList formats = mimeData->formats();
    if (formats.empty() || formats.size() > 50) [[unlikely]] {
        return false;
    }

    QString text;
    if (mimeData->hasUrls()) {
        QStringList urlText;
        for (const QList<QUrl> urls = mimeData->urls(); const QUrl &url : urls) {
            urlText.append(url.toString(QUrl::FullyEncoded));
        }
        text = urlText.join(u' ');
    } else {
        text = mimeData->text();
    }

    auto item = std::make_shared<HistoryItem>(uuid, formats, text);
    auto updateJob = UpdateDatabaseJob::updateClipboard(this, &m_db, m_dbFolder, uuid, text, mimeData, timestamp);
    if (item->type() == HistoryItemType::Image) {
        connect(updateJob, &KJob::finished, this, [this, uuid](KJob *job) {
            if (job->error()) {
                return;
            }
            if (int row = indexOf(uuid); row >= 0) {
                Q_EMIT dataChanged(index(row), index(row), {Qt::DisplayRole, ImageUrlRole, ImageSizeRole});
            }
        });
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(std::move(item));
    endInsertRows();

    // BUG 417590: Remove only after an item is inserted to avoid clearing clipboard
    if (m_items.size() > m_maxSize) {
        // Find the first non-starred item from the end to remove, skipping starred items
        int itemToRemove = -1;
        for (qsizetype i = m_items.size() - 1; i >= 0; --i) {
            // Check if item is starred
            QSqlQuery query(m_db);
            query.prepare(u"SELECT starred FROM main WHERE uuid = ?"_s);
            query.addBindValue(m_items[i]->uuid());
            bool isStarred = false;
            if (query.exec() && query.isSelect() && query.next()) {
                isStarred = query.value(0).toBool();
            }

            if (!isStarred) {
                itemToRemove = i;
                break;
            }
        }

        if (itemToRemove >= 0) {
            removeRow(itemToRemove);
        }
        // If no non-starred items found, we don't remove anything
        // This means starred items can cause the history to exceed maxSize
        // TODO: Consider implementing a separate starred item limit or warning mechanism
    }

    ++m_pendingJobs;
    connect(updateJob, &KJob::finished, this, [this] {
        --m_pendingJobs;
    });
    updateJob->start();

    return true;
}

bool HistoryModel::insert(const QString &text)
{
    auto data = std::make_unique<QMimeData>();
    data->setText(text);
    return insert(data.get());
}

bool HistoryModel::loadHistory()
{
    constexpr const char *failedLoadWarning = "Failed to load history resource. Clipboard history cannot be read.";
    // don't use "appdata", klipper is also a kicker applet
    if (qEnvironmentVariableIsSet("KLIPPER_DATABASE")) {
        m_dbFolder = QFileInfo(qEnvironmentVariable("KLIPPER_DATABASE")).absoluteDir().absolutePath();
    } else {
        m_dbFolder = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper";
    }
    QDir dataDir(m_dbFolder + u"/data");
    dataDir.mkpath(dataDir.absolutePath());

    // Try to reuse the previous connection
    m_db = QSqlDatabase::database(u"klipper"_s);
    if (!m_db.isValid()) [[likely]] {
        m_db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"klipper"_s);
        m_db.setHostName(u"localhost"_s);
        if (qEnvironmentVariableIsSet("KLIPPER_DATABASE")) {
            m_db.setDatabaseName(qEnvironmentVariable("KLIPPER_DATABASE"));
        } else {
            m_db.setDatabaseName(m_dbFolder + u"/history3.sqlite");
        }
    }
    if (!m_db.isOpen() && !m_db.open()) {
        qCWarning(KLIPPER_LOG) << failedLoadWarning << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    query.exec(u"PRAGMA journal_mode=WAL"_s);
    // The main table only stores text data
    query.exec(
        u"CREATE TABLE IF NOT EXISTS main (uuid char(40) PRIMARY KEY, added_time REAL NOT NULL CHECK (added_time > 0), last_used_time REAL CHECK (last_used_time > 0), mimetypes TEXT NOT NULL, text NTEXT, starred BOOLEAN)"_s);
    // The aux table stores data index
    query.exec(u"CREATE TABLE IF NOT EXISTS aux (uuid char(40) NOT NULL, mimetype TEXT NOT NULL, data_uuid char(40) NOT NULL, PRIMARY KEY (uuid, mimetype))"_s);
    // Save the latest version number
    query.exec(u"CREATE TABLE IF NOT EXISTS version (db_version INT NOT NULL)"_s);
    constexpr int currentDBVersion = 3;
    if (query.exec(u"SELECT db_version FROM version"_s) && query.isSelect() && query.next() /* has a record */) {
        if (query.value(0).toInt() != currentDBVersion) {
            return false;
        }
    } else if (!query.exec(u"INSERT INTO version (db_version) VALUES (%1)"_s.arg(QString::number(currentDBVersion)))) {
        qCWarning(KLIPPER_LOG) << failedLoadWarning << m_db.lastError().text();
        return false;
    }

    if (!m_bKeepContents) {
        QSqlQuery clearQuery(m_db);
        clearQuery.exec(u"DELETE FROM main"_s);
        clearQuery.exec(u"DELETE FROM aux"_s);
        clearQuery.exec(u"VACUUM"_s);
        if (dataDir.exists()) {
            dataDir.removeRecursively();
            dataDir.mkpath(dataDir.absolutePath());
        }
    }

    if (m_maxSize == 0) {
        return true;
    }

    // The last row is either items.size() - 1 or m_maxSize - 1.
    decltype(m_items) items;
    int starredCount = 0;
    if (query.exec(u"SELECT * FROM main ORDER BY last_used_time DESC, added_time DESC LIMIT %1"_s.arg(QString::number(m_maxSize))) && query.isSelect()) {
        items.reserve(std::max(query.size(), 1));
        while (query.next()) {
            if (HistoryItemPtr item = HistoryItem::create(query)) {
                items.emplace_back(std::move(item));
                starredCount += query.value(u"starred"_s).toBool() ? 1 : 0;
            }
        }
    }

    if (items.empty()) {
        // special case - nothing to insert
        return true;
    }

    beginResetModel();
    m_items = std::move(items);
    m_starredCount = starredCount;
    endResetModel();

    m_clip->setMimeData(m_items[0], SystemClipboard::SelectionMode(SystemClipboard::Clipboard | SystemClipboard::Selection));

    return true;
}

bool HistoryModel::saveClipboardHistory()
{
    QSqlQuery query(u"PRAGMA wal_checkpoint"_s, m_db);
    return query.exec();
}

void HistoryModel::loadSettings()
{
    setMaxSize(KlipperSettings::maxClipItems());
    m_displayImages = !KlipperSettings::ignoreImages();
    m_bNoNullClipboard = KlipperSettings::preventEmptyClipboard();
    // 0 is the id of "Ignore selection" radiobutton
    m_bIgnoreSelection = KlipperSettings::ignoreSelection();
    m_bKeepContents = KlipperSettings::keepClipboardContents();
    m_bSynchronize = KlipperSettings::syncClipboards();
    m_bSelectionTextOnly = KlipperSettings::selectionTextOnly();

    if (m_bNoNullClipboard) {
        connect(m_clip.get(), &SystemClipboard::receivedEmptyClipboard, this, &HistoryModel::slotReceivedEmptyClipboard, Qt::UniqueConnection);
    } else {
        disconnect(m_clip.get(), &SystemClipboard::receivedEmptyClipboard, this, &HistoryModel::slotReceivedEmptyClipboard);
    }

    // BUG: 142882
    // Security: If user has save clipboard turned off, old data should be deleted from disk
    if (!m_bKeepContents) {
        clear();
    }
}

int HistoryModel::pendingJobs() const
{
    return m_pendingJobs;
}

void HistoryModel::moveToTop(const QString &uuid)
{
    const int existingItemIndex = indexOf(uuid);
    if (existingItemIndex < 0) {
        return;
    }
    moveToTop(existingItemIndex);
}

KCoreConfigSkeleton *HistoryModel::settings()
{
    return KlipperSettings::self();
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
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
}

void HistoryModel::saveToFile(QStringView dbFolder, const QByteArray &data, QStringView newUuid, QStringView dataUuid)
{
    const QString folderPath = dbFolder + u"/data/" + newUuid;
    QDir().mkpath(folderPath);
    QSaveFile file(folderPath + u'/' + dataUuid);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(KLIPPER_LOG) << file.errorString() << folderPath;
        return;
    }
    file.write(data);
    file.commit();
}

void HistoryModel::moveTopToBack()
{
    if (m_items.size() < 2) {
        return;
    }
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
    hash.insert(ImageUrlRole, QByteArrayLiteral("decoration"));
    hash.insert(ImageSizeRole, QByteArrayLiteral("imageSize"));
    hash.insert(UuidRole, QByteArrayLiteral("uuid"));
    hash.insert(TypeIntRole, QByteArrayLiteral("type"));
    hash.insert(StarredRole, QByteArrayLiteral("starred"));
    return hash;
}

void HistoryModel::checkClipData(QClipboard::Mode mode, const QMimeData *data)
{
    Q_ASSERT(m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard));
    bool changed = true; // ### FIXME (only relevant under polling, might be better to simply remove polling and rely on XFixes)

    // this must be below the "bNoNullClipboard" handling code!
    // XXX: I want a better handling of selection/clipboard in general.
    // XXX: Order sensitive code. Must die.
    const bool selectionMode = mode == QClipboard::Selection;
    if (selectionMode && m_bIgnoreSelection) {
        if (m_bSynchronize) {
            m_clip->setMimeData(data, SystemClipboard::Clipboard, SystemClipboard::ClipboardUpdateReason::SyncSelection);
        }
        return;
    }

    if (selectionMode && m_bSelectionTextOnly && !data->hasText()) {
        return;
    }

    if (data->data(QStringLiteral("x-kde-passwordManagerHint")) == QByteArrayView("secret")) {
        return;
    }

    if (!m_displayImages && data->hasImage() && !data->hasText() /*BUG 491488*/ && !data->hasFormat(QStringLiteral("x-kde-force-image-copy"))) {
        return;
    }

    if (changed && insert(data)) [[likely]] {
        qCDebug(KLIPPER_LOG) << "Synchronize?" << m_bSynchronize;
        if (m_bSynchronize) { // applyClipChanges can return nullptr
            m_clip->setMimeData(data, mode == QClipboard::Selection ? SystemClipboard::Clipboard : SystemClipboard::Selection);
        }
    }
}

void HistoryModel::slotIgnored(QClipboard::Mode mode)
{
    // internal to klipper, ignoring QSpinBox selections
    // keep our old clipboard, thanks
    // This won't quite work, but it's close enough for now.
    // The trouble is that the top selection =! top clipboard
    // but we don't track that yet. We will....
    if (auto top = first()) {
        m_clip->setMimeData(top, mode == QClipboard::Selection ? SystemClipboard::Selection : SystemClipboard::Clipboard);
    }
}

void HistoryModel::slotReceivedEmptyClipboard(QClipboard::Mode mode)
{
    Q_ASSERT(m_bNoNullClipboard);
    if (auto top = first()) {
        // keep old clipboard after someone set it to null
        qCDebug(KLIPPER_LOG) << "Resetting clipboard (Prevent empty clipboard)";
        m_clip->setMimeData(top,
                            mode == QClipboard::Selection ? SystemClipboard::Selection : SystemClipboard::Clipboard,
                            SystemClipboard::ClipboardUpdateReason::PreventEmptyClipboard);
    }
}

bool HistoryModel::isItemStarred(const QString &uuid) const
{
    QSqlQuery query(m_db);
    query.prepare(u"SELECT starred FROM main WHERE uuid = ?"_s);
    query.addBindValue(uuid);
    if (query.exec() && query.isSelect() && query.next()) {
        return query.value(0).toBool();
    }
    return false;
}

#include "moc_historymodel.cpp"
