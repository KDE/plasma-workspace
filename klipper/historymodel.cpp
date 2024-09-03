/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historymodel.h"

#include <chrono>
#include <zlib.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QMimeData>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtConcurrentRun>

#include <KIO/DeleteJob>
#include <KLocalizedString>
#include <KMessageBox>

#include "config-klipper.h"
#include "historyitem.h"
#include "klipper_debug.h"
#include "klippersettings.h"
#include "systemclipboard.h"

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
        return QString::fromLatin1(hash.result().toHex());
    } else if (data->hasUrls()) {
        for (const auto urls = data->urls(); const QUrl &url : urls) {
            hash.addData(url.toEncoded());
            hash.addData("\0", 1); // Use binary zero as that is not a valid path character
        }
        QByteArray buffer;
        hash.addData(buffer);
        return QString::fromLatin1(hash.result().toHex());
    } else if (data->hasImage()) {
        const QImage image = data->imageData().value<QImage>();
        hash.addData(QByteArray::fromRawData(reinterpret_cast<const char *>(image.constBits()), image.sizeInBytes()));
        return QString::fromLatin1(hash.result().toHex());
    }
    return QString();
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
            return;
        }
        if (!isTop) {
            return;
        }

        QSqlQuery query(m_db);
        query.prepare(u"UPDATE main SET last_used_time=? WHERE uuid='%1'"_s.arg(m_items[0]->uuid()));
        query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000.0);
        if (TransactionGuard transaction(&m_db); transaction.exec(query)) {
            if (m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
                return;
            }
            m_clip->setMimeData(m_items[0], SystemClipboard::SelectionMode(SystemClipboard::Clipboard | SystemClipboard::Selection));
        }
    });

    connect(m_clip.get(), &SystemClipboard::ignored, this, &HistoryModel::slotIgnored);
    connect(m_clip.get(), &SystemClipboard::newClipData, this, &HistoryModel::checkClipData);
    qCritical() << "m1";
}

HistoryModel::~HistoryModel()
{
}

void HistoryModel::clear()
{
    if (TransactionGuard transaction(&m_db); !transaction.exec(u"DELETE FROM main"_s) || !transaction.exec(u"DELETE FROM aux"_s)) {
        return;
    }
    qCritical() << "m2";
    for (const auto &item : m_items) {
        KIO::del(QUrl::fromLocalFile(m_dbFolder + u"/data/" + item->uuid() + u'/'));
    }
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
        if (!(item->allTypes() & HistoryItemType::Image)) {
            return QUrl();
        }
        QSqlQuery query(m_db);
        query.prepare(
            u"SELECT uuid,mimetype,data_uuid FROM aux WHERE uuid='%1' AND (mimetype LIKE 'image/%' OR mimetype='application/x-qt-image')"_s.arg(item->uuid()));
        if (query.exec() && query.isSelect() && query.next()) {
            return QUrl::fromLocalFile(m_dbFolder + u"/data/" + item->uuid() + u'/' + query.value(2).toString());
        }
        return QUrl();
    }
    case HistoryItemConstPtrRole:
        return QVariant::fromValue<HistoryItemConstPtr>(std::const_pointer_cast<const HistoryItem>(item));
    case UuidRole:
        return item->uuid();
    case TypeRole:
        return QVariant::fromValue<HistoryItemType>(item->type());
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
        if (item->type() != HistoryItemType::Text || !value.canConvert<QString>()) [[unlikely]] {
            return false;
        }
        if (!m_db.isOpen()) {
            qCWarning(KLIPPER_LOG) << m_db.lastError().text();
            return false;
        }

        QString text = value.toString();
        QString newUuid = QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1));
        {
            TransactionGuard transaction(&m_db);
            {
                QSqlQuery query(m_db);
                query.prepare(u"UPDATE main SET (uuid, mimetypes, text)=(?, ?, ?) WHERE uuid='%1'"_s.arg(item->uuid()));
                query.addBindValue(newUuid);
                query.addBindValue(u"text/plain"_s);
                query.addBindValue(text);
                // last_used_time is updated in the signal slot
                if (!transaction.exec(query) || !transaction.exec(u"DELETE FROM aux WHERE uuid='%1'"_s.arg(item->uuid()))) {
                    return false;
                }
            }

            QSqlQuery query(m_db);
            query.prepare(u"INSERT INTO aux (uuid, mimetype, data_uuid) VALUES (?, ?, ?)"_s);
            query.addBindValue(newUuid);
            query.addBindValue(u"text/plain"_s);
            query.addBindValue(newUuid);
            if (!transaction.exec(query)) {
                return false;
            }
        }

        KIO::del(QUrl::fromLocalFile(m_dbFolder + u"/data/" + item->uuid() + u'/'));
        QThreadPool::globalInstance()->start([this, oldUuid = item->uuid(), newUuid, data = text.toUtf8()] {
            saveToFile(data, newUuid, newUuid);
        });

        item = std::make_shared<HistoryItem>(std::move(newUuid), QStringList{u"text/plain"_s}, std::move(text));
        Q_EMIT dataChanged(index, index, {Qt::DisplayRole});
        return true;
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
    qCritical() << "m3";
    for (int i = 0; i < count; ++i) {
        KIO::del(QUrl::fromLocalFile(m_dbFolder + u"/data/" + m_items[row + i]->uuid() + u'/'));
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    m_items.erase(std::next(m_items.cbegin(), row), std::next(m_items.cbegin(), row + count));
    endRemoveRows();
    return true;
}

bool HistoryModel::remove(const QString &uuid)
{
    const int index = indexOf(uuid);
    if (index < 0) {
        return false;
    }
    return removeRow(index, QModelIndex());
}

int HistoryModel::indexOf(const QString &uuid) const
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

bool HistoryModel::insert(const QMimeData *mimeData, qreal timestamp)
{
    if (m_maxSize == 0) {
        // special case - cannot insert any items
        return false;
    }
    qCritical() << "insert1";
    QString uuid = computeUuid(mimeData);
    if (uuid.isEmpty()) {
        return false;
    }
    qCritical() << "insert2";
    if (const int existingItemIndex = indexOf(uuid); existingItemIndex >= 0) {
        // move to top
        moveToTop(existingItemIndex);
        return true;
    }
    qCritical() << "insert3";
    QString text;
    if (mimeData->hasUrls()) {
        QStringList urlText;
        for (const QList<QUrl> urls = mimeData->urls(); const QUrl &url : urls) {
            urlText.append(url.toString(QUrl::FullyEncoded));
        }
        text = urlText.join(u' ');
    } else if (mimeData->hasImage() && !mimeData->hasText()) {
        const QImage image = mimeData->imageData().value<QImage>();
        text = u"▨ " + i18n("%1x%2 %3bpp", image.width(), image.height(), image.depth());
    } else {
        text = mimeData->text();
    }
    qCritical() << "insert4";
    QStringList formats = mimeData->formats();
    {
        TransactionGuard transaction(&m_db);
        {
            QSqlQuery query(m_db);
            query.prepare(u"INSERT INTO main (uuid, added_time, mimetypes, text) VALUES (?, ?, ?, ?)"_s);
            query.addBindValue(uuid);
            if (timestamp == 0) [[likely]] {
                query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000.0);
            } else {
                query.addBindValue(qreal(timestamp));
            }
            query.addBindValue(formats.join(u','));
            query.addBindValue(text);
            if (!transaction.exec(query)) {
                return false;
            }
        }

        QString imageUuid;
        QCryptographicHash hash(QCryptographicHash::Sha1);
        for (const QString &format : std::as_const(formats)) {
            if (!format.contains(u'/')) {
                continue;
            }
            QSqlQuery query(m_db);
            query.prepare(u"INSERT INTO aux (uuid, mimetype, data_uuid) VALUES (?, ?, ?)"_s);
            query.addBindValue(uuid);
            query.addBindValue(format);

            hash.reset();
            QByteArray data;
            if (format.startsWith(u"image/") || format == u"application/x-qt-image") {
                if (!imageUuid.isEmpty()) {
                    query.addBindValue(imageUuid);
                } else {
                    QImage image = mimeData->imageData().value<QImage>();
                    hash.addData(QByteArray::fromRawData(reinterpret_cast<const char *>(image.constBits()), image.sizeInBytes()));
                    imageUuid = QString::fromLatin1(hash.result().toHex());
                    query.addBindValue(imageUuid);
                    QThreadPool::globalInstance()->start([this, image = std::move(image), uuid, imageUuid] {
                        const QString folderPath = m_dbFolder + u"/data/" + uuid;
                        QDir().mkpath(folderPath);
                        image.save(folderPath + u'/' + imageUuid, "PNG");
                    });
                }
            } else {
                data = mimeData->data(format);
                hash.addData(data);
                QString dataUuid = QString::fromLatin1(hash.result().toHex());
                query.addBindValue(dataUuid);
                // Start a job to save the data to a file
                QThreadPool::globalInstance()->start([this, data = std::move(data), uuid, dataUuid = std::move(dataUuid)] {
                    saveToFile(data, uuid, dataUuid);
                });
            }
            if (!transaction.exec(query)) {
                return false;
            }
        }
    }
    qCritical() << "insert5";

    if (m_items.size() > m_maxSize - 1) {
        if (!removeRow(m_items.size() - 1)) [[unlikely]] {
            return false;
        }
    }
    qCritical() << "insert6";
    beginInsertRows(QModelIndex(), 0, 0);
    qCritical() << "insert6a";
    m_items.prepend(std::make_shared<HistoryItem>(std::move(uuid), std::move(formats), std::move(text)));
    qCritical() << "insert6b";
    endInsertRows();
    qCritical() << "insert7";
    return true;
}

bool HistoryModel::insert(const QString &text)
{
    auto data = new QMimeData;
    data->setText(text);
    return insert(data);
}

bool HistoryModel::loadHistory()
{
    if (m_maxSize == 0 || !KlipperSettings::keepClipboardContents()) [[unlikely]] {
        // rare special case - cannot insert any items
        return true;
    }

    constexpr const char *failedLoadWarning = "Failed to load history resource. Clipboard history cannot be read.";
    // don't use "appdata", klipper is also a kicker applet
    // Try to reuse the previous connection
    if (qEnvironmentVariableIsSet("KLIPPER_DATABASE")) {
        m_dbFolder = QFileInfo(qEnvironmentVariable("KLIPPER_DATABASE")).absoluteDir().absolutePath();
    } else {
        m_dbFolder = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper";
        QDir().mkpath(m_dbFolder);
    }
    m_db = QSqlDatabase::database(u"klipper"_s);
    if (!m_db.isValid()) [[likely]] {
        m_db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"klipper"_s);
        m_db.setHostName(u"localhost"_s);
        if (qEnvironmentVariableIsSet("KLIPPER_DATABASE")) {
            m_db.setDatabaseName(qEnvironmentVariable("KLIPPER_DATABASE"));
        } else {
            m_db.setDatabaseName(m_dbFolder + u"/history3.sqlite");
        }
        if (!m_db.open()) {
            qCWarning(KLIPPER_LOG) << failedLoadWarning << m_db.lastError().text();
            return false;
        }
    }

    QSqlQuery query(m_db);
    // The main table only stores text data
    query.exec(
        u"CREATE TABLE IF NOT EXISTS main (uuid char(40) PRIMARY KEY, added_time REAL NOT NULL, last_used_time REAL, mimetypes TEXT NOT NULL, text NTEXT, starred BOOLEAN)"_s);
    // The aux table stores data index
    query.exec(u"CREATE TABLE IF NOT EXISTS aux (uuid char(40) NOT NULL, mimetype TEXT NOT NULL, data_uuid char(40), PRIMARY KEY (uuid,mimetype))"_s);
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

    // The last row is either items.size() - 1 or m_maxSize - 1.
    decltype(m_items) items;
    if (query.exec(u"SELECT * FROM main ORDER BY last_used_time DESC, added_time DESC LIMIT %1"_s.arg(QString::number(m_maxSize))) && query.isSelect()) {
        while (query.next()) {
            if (HistoryItemPtr item = HistoryItem::create(query)) {
                items.emplace_back(std::move(item));
            }
        }
    }

    if (items.empty()) {
        // special case - nothing to insert, so just clear.
        clear();
        return true;
    }

    beginResetModel();
    m_items = std::move(items);
    endResetModel();

    m_clip->setMimeData(m_items[0], SystemClipboard::SelectionMode(SystemClipboard::Clipboard | SystemClipboard::Selection));

    return true;
}

void HistoryModel::loadSettings()
{
    setMaxSize(KlipperSettings::maxClipItems());
    m_displayImages = !KlipperSettings::ignoreImages();
    m_bNoNullClipboard = KlipperSettings::preventEmptyClipboard();
    // 0 is the id of "Ignore selection" radiobutton
    m_bIgnoreSelection = KlipperSettings::ignoreSelection();
    m_bSynchronize = KlipperSettings::syncClipboards();
    m_bSelectionTextOnly = KlipperSettings::selectionTextOnly();

    if (m_bNoNullClipboard) {
        connect(m_clip.get(), &SystemClipboard::receivedEmptyClipboard, this, &HistoryModel::slotReceivedEmptyClipboard, Qt::UniqueConnection);
    } else {
        disconnect(m_clip.get(), &SystemClipboard::receivedEmptyClipboard, this, &HistoryModel::slotReceivedEmptyClipboard);
    }
}

void HistoryModel::moveToTop(const QString &uuid)
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
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
}

void HistoryModel::saveToFile(const QByteArray &data, QStringView newUuid, QStringView dataUuid)
{
    const QString folderPath = m_dbFolder + u"/data/" + newUuid;
    QDir().mkpath(folderPath);
    QSaveFile file(folderPath + u'/' + dataUuid);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(KLIPPER_LOG) << file.errorString() << folderPath;
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
    hash.insert(Qt::DecorationRole, QByteArrayLiteral("decoration"));
    hash.insert(UuidRole, QByteArrayLiteral("uuid"));
    hash.insert(TypeIntRole, QByteArrayLiteral("type"));
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
