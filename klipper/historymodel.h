/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QRecursiveMutex>
#include <QTimer>

#include "klipper_export.h"

class HistoryItem;

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
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;
    bool remove(const QByteArray &uuid);

    int maxSize() const;
    void setMaxSize(int size);

    bool displayImages() const;
    void setDisplayImages(bool show);

    /**
     * Clear history
     */
    void clear();
    void clearHistory();
    void moveToTop(const QByteArray &uuid);
    void moveTopToBack();
    void moveBackToTop();

    int indexOf(const QByteArray &uuid) const;
    int indexOf(const HistoryItem *item) const;

    void insert(const std::shared_ptr<HistoryItem> &item);

    QRecursiveMutex *mutex()
    {
        return &m_mutex;
    }

    /**
     * @short Loads history from disk.
     * Inserts items into clipboard without any checks
     * Used when restoring a saved history and internally.
     * Don't use this unless you're reasonable the list
     * should be reset.
     */
    bool loadHistory();

    /**
     * Save history to disk
     */
    bool saveHistory(bool empty = false);
    void startSaveHistoryTimer(std::chrono::seconds delay = std::chrono::seconds(5));

private:
    explicit HistoryModel();

    void moveToTop(int row);
    QList<std::shared_ptr<HistoryItem>> m_items;
    int m_maxSize;
    bool m_displayImages;
    QRecursiveMutex m_mutex;

    QTimer m_saveFileTimer;
};

inline int HistoryModel::maxSize() const
{
    return m_maxSize;
}

inline bool HistoryModel::displayImages() const
{
    return m_displayImages;
}

inline void HistoryModel::setDisplayImages(bool show)
{
    m_displayImages = show;
}
