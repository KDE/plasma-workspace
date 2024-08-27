/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QBindable>
#include <QRecursiveMutex>
#include <QTimer>

#include "historyitem.h"
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
    HistoryItemSharedPtr first() const;

    /**
     * Inserts item into clipboard history top
     * if duplicate entry exist, the older duplicate is deleted.
     * The duplicate concept is "deep", so that two text string
     * are considerd duplicate if identical.
     */
    void insert(const HistoryItemSharedPtr &item);

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

    void actionInvoked(const HistoryItemSharedPtr &item);

private:
    explicit HistoryModel();

    void moveToTop(qsizetype row);

    std::shared_ptr<SystemClipboard> m_clip;
    QList<HistoryItemSharedPtr> m_items;
    qsizetype m_maxSize = 0;
    bool m_displayImages;
    QRecursiveMutex m_mutex;

    QTimer m_saveFileTimer;

    friend class DeclarativeHistoryModel;
};
