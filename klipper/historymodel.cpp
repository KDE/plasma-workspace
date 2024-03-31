/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historymodel.h"
#include "historyimageitem.h"
#include "historystringitem.h"
#include "historyurlitem.h"
#include "klipper.h"

#include <QApplication>

#include <KSystemClipboard>
#include <KWindowSystem>

#if HAVE_X11
#include <xcb/xcb_event.h>
#endif // HAVE_X11
#include <wayland-client-core.h> // roundtrip

namespace
{
bool ignoreClipboardChanges()
{
    // Changing a spinbox in klipper's config-dialog causes the lineedit-contents
    // of the spinbox to be selected and hence the clipboard changes. But we don't
    // want all those items in klipper's history. See #41917
    const auto app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app) {
        return false;
    }

    QWidget *focusWidget = app->focusWidget();
    if (focusWidget) {
        if (focusWidget->inherits("QSpinBox")
            || (focusWidget->parentWidget() && focusWidget->inherits("QLineEdit") && focusWidget->parentWidget()->inherits("QSpinWidget"))) {
            return true;
        }
    }

    return false;
}

void updateTimestamp()
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        QX11Info::setAppTime(QX11Info::getTimestamp());
    }
#endif
}

void roundtrip()
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        const auto cookie = xcb_get_input_focus(m_x11Interface->connection());
        xcb_generic_error_t *error = nullptr;
        QScopedPointer<xcb_get_input_focus_reply_t, QScopedPointerPodDeleter> sync(xcb_get_input_focus_reply(m_x11Interface->connection(), cookie, &error));
        if (error) {
            free(error);
        }
    } else
#endif
        if (KWindowSystem::isPlatformWayland()) {
        wl_display_roundtrip(qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()->display());
    }
}
}

std::shared_ptr<HistoryModel> self()
{
    static std::weak_ptr<HistoryModel> s_model;
    if (s_model.expired()) {
        std::shared_ptr<HistoryModel> ptr{new HistoryModel};
        s_model = ptr;
        return ptr;
    }
    return s_model.lock();
}

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_maxSize(0)
    , m_displayImages(true)
{
    updateTimestamp(); // read initial X user time
    m_clip = KSystemClipboard::instance();
    connect(m_clip, &KSystemClipboard::changed, this, &HistoryModel::onNewClipData);
}

HistoryModel::~HistoryModel()
{
    clear();
}

void HistoryModel::clear()
{
    const std::lock_guard lock(&m_mutex);
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void HistoryModel::setMaxSize(int size)
{
    if (m_maxSize == size) {
        return;
    }
    const std::lock_guard lock(&m_mutex);
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
    return m_items.size()();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::DoNotUseParent)) {
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
    const std::lock_guard lock(&m_mutex);
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_items.removeAt(row);
    }
    endRemoveRows();
    return true;
}

bool HistoryModel::remove(const QByteArray &uuid)
{
    QModelIndex index = indexOf(uuid);
    if (!index.isValid()) {
        return false;
    }
    return removeRow(index.row(), QModelIndex());
}

QModelIndex HistoryModel::indexOf(const QByteArray &uuid) const
{
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items.at(i)->uuid() == uuid) {
            return index(i);
        }
    }
    return QModelIndex();
}

QModelIndex HistoryModel::indexOf(const HistoryItem *item) const
{
    if (!item) {
        return QModelIndex();
    }
    return indexOf(item->uuid());
}

void HistoryModel::insert(const std::shared_ptr<HistoryItem> &item)
{
    if (!item) [[unlikely]] {
        Q_ASSERT(false);
        return;
    }

    if (m_maxSize == 0) {
        // special case - cannot insert any items
        return;
    }

    const std::lock_guard lock(&m_mutex);

    const QModelIndex existingItem = indexOf(item.get());
    if (existingItem.isValid()) {
        // move to top
        moveToTop(existingItem.row());
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

void HistoryModel::clearAndBatchInsert(const QList<HistoryItemPtr> &items)
{
    if (m_maxSize == 0) {
        // special case - cannot insert any items
        return;
    }

    if (items.empty()) {
        // special case - nothing to insert, so just clear.
        clear();
        return;
    }

    QMutexLocker lock(&m_mutex);

    beginResetModel();
    m_items.clear();

    // The last row is either items.size() - 1 or m_maxSize - 1.
    const int numOfItemsToBeInserted = std::min(static_cast<int>(items.size()), m_maxSize);
    m_items.reserve(numOfItemsToBeInserted);

    for (int i = 0; i < numOfItemsToBeInserted; i++) {
        if (!items[i]) {
            continue;
        }

        items[i]->setModel(this);
        m_items.append(items[i]);
    }

    endResetModel();
}

HistoryItemConstPtr HistoryModel::first() const
{
    if (m_items.empty()) {
        return HistoryItemConstPtr();
    }
    return m_items[0];
}

void HistoryModel::moveToTop(const QByteArray &uuid)
{
    const QModelIndex existingItem = indexOf(uuid);
    if (!existingItem.isValid()) {
        return;
    }
    moveToTop(existingItem.row());
}

void HistoryModel::onNewClipData(QClipboard::Mode mode)
{
    if ((mode == QClipboard::Clipboard && m_clipboardLocklevel) || (mode == QClipboard::Selection && m_selectionLocklevel)) {
        return;
    }

    if (mode == QClipboard::Selection && blockFetchingNewData()) {
        return;
    }

    const bool isSelectionMode = mode == QClipboard::Selection;
    // internal to klipper, ignoring QSpinBox selections
    if (ignoreClipboardChanges()) {
        // keep our old clipboard, thanks
        // This won't quite work, but it's close enough for now.
        // The trouble is that the top selection =! top clipboard
        // but we don't track that yet. We will....
        if (!m_items.empty()) {
            setClipboard(m_items[0], isSelectionMode ? Selection : Clipboard, ClipboardUpdateReason::UpdateClipboard);
        }
        return;
    }

    qCDebug(KLIPPER_LOG) << "Checking clip data";

    QMimeData *const data = m_clip->mimeData(isSelectionMode ? QClipboard::Selection : QClipboard::Clipboard);

    bool clipEmpty = false;
    if (!data) {
        clipEmpty = true;
    } else {
        clipEmpty = data->formats().isEmpty();
        if (clipEmpty) {
            // Might be a timeout. Try again
            roundtrip();
            data = m_clip->mimeData(isSelectionMode ? QClipboard::Selection : QClipboard::Clipboard);
            clipEmpty = data->formats().isEmpty();
            qCDebug(KLIPPER_LOG) << "was empty. Retried, now " << (clipEmpty ? " still empty" : " no longer empty");
        }
    }

    if (clipEmpty && m_bNoNullClipboard) {
        if (!m_items.empty()) {
            // Keep old clipboard after someone set it to null
            qCDebug(KLIPPER_LOG) << "Resetting clipboard (Prevent empty clipboard)";
            setClipboard(m_items[0], isSelectionMode ? Selection : Clipboard, ClipboardUpdateReason::PreventEmptyClipboard);
        }
        return;
    } else if (clipEmpty) {
        return;
    }

    // this must be below the "bNoNullClipboard" handling code!
    // XXX: I want a better handling of selection/clipboard in general.
    // XXX: Order sensitive code. Must die.
    if (isSelectionMode && (m_bIgnoreSelection || (m_bSelectionTextOnly && !data->hasText()))) {
        return;
    }

    if (data->hasUrls()) {
        ; // ok
    } else if (data->hasText()) {
        ; // ok
    } else if (data->hasImage()) {
        if (m_bIgnoreImages && !data->hasFormat(u"x-kde-force-image-copy"_s)) {
            return;
        }
    } else {
        return; // unknown, ignore
    }

    QString &lastURLGrabberText = isSelectionMode ? m_lastURLGrabberTextSelection : m_lastURLGrabberTextClipboard;
    if ((isSelectionMode && m_selectionLocklevel) || (!isSelectionMode && m_clipboardLocklevel)) {
        lastURLGrabberText.clear();
        return;
    }

    HistoryItemPtr item;
    {
        Ignore lock(isSelectionMode ? m_selectionLocklevel : m_clipboardLocklevel);

        if (!m_items.empty()) {
            if (m_bIgnoreImages && m_items[0]->type() == HistoryItemType::Image) {
                beginRemoveRows(QModelIndex(), 0, 0);
                m_items.removeAt(0);
                endRemoveRows();
            }
        }

        item = HistoryItem::create(data);

        const bool saveToHistory = clipData->data(u"x-kde-passwordManagerHint"_s) == QByteArrayView("secret");
        if (saveToHistory) {
            insert(item);
        }
    }

    qCDebug(KLIPPER_LOG) << "Synchronize?" << m_bSynchronize;
    if (m_bSynchronize) {
        setClipboard(item, isSelectionMode ? Clipboard : Selection);
    }

    if (m_bURLGrabber && data->hasText()) {
        m_myURLGrabber->checkNewData(std::const_pointer_cast<const HistoryItem>(item));

        // Make sure URLGrabber doesn't repeat all the time if klipper reads the same
        // text all the time (e.g. because XFixes is not available and the application
        // has broken TIMESTAMP target). Using most recent history item may not always
        // work.
        if (item->text() != lastURLGrabberText) {
            lastURLGrabberText = item->text();
        }
    } else {
        lastURLGrabberText.clear();
    }
}

void HistoryModel::moveToTop(int row)
{
    if (row == 0 || row >= m_items.count()) {
        return;
    }
    const std::lock_guard lock(&m_mutex);
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
}

void HistoryModel::moveTopToBack()
{
    if (m_items.count() < 2) {
        return;
    }
    const std::lock_guard lock(&m_mutex);
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
    hash.insert(Qt::DisplayRole, QByteArrayLiteral("DisplayRole"));
    hash.insert(Qt::DecorationRole, QByteArrayLiteral("DecorationRole"));
    hash.insert(Base64UuidRole, QByteArrayLiteral("UuidRole"));
    hash.insert(TypeIntRole, QByteArrayLiteral("TypeRole"));
    return hash;
}
