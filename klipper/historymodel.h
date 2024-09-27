/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QBindable>
#include <QClipboard>
#include <QRecursiveMutex>
#include <QTimer>

#include "klipper_export.h"

class HistoryItem;
class SystemClipboard;

class KLIPPER_EXPORT HistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum RoleType {
        HistoryItemConstPtrRole = Qt::UserRole,
        UuidRole,
        TypeRole,
        Base64UuidRole,
        TypeIntRole,
    };
    Q_ENUM(RoleType)

    [[nodiscard]] static std::shared_ptr<HistoryModel> self();
    ~HistoryModel() override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * Remove (first) history item equal to item from history
     */
    bool remove(const QByteArray &uuid);

    qsizetype maxSize() const;
    void setMaxSize(qsizetype size);

    /**
     * Clear history
     */
    void clear();
    void clearHistory();

    /**
     * Move the history in position pos to top
     */
    void moveToTop(const QByteArray &uuid);
    void moveTopToBack();
    void moveBackToTop();

    int indexOf(const QByteArray &uuid) const;
    int indexOf(const HistoryItem *item) const;

    /**
     * Traversal: Get first item
     */
    std::shared_ptr<const HistoryItem> first() const;

    /**
     * Inserts item into clipboard history top
     * if duplicate entry exist, the older duplicate is deleted.
     * The duplicate concept is "deep", so that two text string
     * are considerd duplicate if identical.
     */
    void insert(const std::shared_ptr<HistoryItem> &item);

    /**
     * @short Loads history from disk.
     * Inserts items into clipboard without any checks
     * Used when restoring a saved history and internally.
     * Don't use this unless you're reasonable the list
     * should be reset.
     */
    bool loadHistory();

    void loadSettings();

    /**
     * Save history to disk
     */
    bool saveHistory(bool empty = false);
    void startSaveHistoryTimer(std::chrono::seconds delay = std::chrono::seconds(5));

Q_SIGNALS:
    void changed(bool isTop = false);

    void actionInvoked(const std::shared_ptr<const HistoryItem> &item);

private Q_SLOTS:
    /**
     * Check data in clipboard, and if it passes these checks,
     * store the data in the clipboard history.
     */
    void checkClipData(QClipboard::Mode mode, const QMimeData *data);

    void slotIgnored(QClipboard::Mode mode);
    void slotReceivedEmptyClipboard(QClipboard::Mode mode);

private:
    explicit HistoryModel();

    /**
     * Enter clipboard data in the history.
     */
    [[nodiscard]] std::shared_ptr<HistoryItem> applyClipChanges(const QMimeData *data);

    void moveToTop(qsizetype row);

    std::shared_ptr<SystemClipboard> m_clip;
    QList<std::shared_ptr<HistoryItem>> m_items;
    qsizetype m_maxSize = 0;
    bool m_displayImages = false;
    bool m_bNoNullClipboard = true;
    bool m_bIgnoreSelection = true;
    bool m_bSynchronize = false;
    bool m_bSelectionTextOnly = true;
    QRecursiveMutex m_mutex;

    QTimer m_saveFileTimer;

    friend class DeclarativeHistoryModel;
};
