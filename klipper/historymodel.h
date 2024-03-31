/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>
#include <mutex>

#include <QAbstractListModel>
#include <QClipboard>

#include "historyitem.h"

class Klipper;
class KSystemClipboard;
class HistoryItem;

class HistoryModel : public QAbstractListModel
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
    explicit HistoryModel(QObject *parent = nullptr);
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

    void clear();
    void moveToTop(const QByteArray &uuid);
    void moveTopToBack();
    void moveBackToTop();

    QModelIndex indexOf(const QByteArray &uuid) const;
    QModelIndex indexOf(const HistoryItem *item) const;

    void insert(const std::shared_ptr<HistoryItem> &item);
    void clearAndBatchInsert(const QList<std::shared_ptr<HistoryItem>> &items);

    const HistoryItemConstPtr &first() const;

private:
    /**
     * The selection modes
     *
     * Don't use 1, as I use that as a guard against passing
     * a boolean true as a mode.
     */
    enum SelectionMode {
        Clipboard = 2,
        Selection = 4,
    };

    enum class ClipboardUpdateReason {
        UpdateClipboard,
        PreventEmptyClipboard,
    };

    void onNewClipData(QClipboard::Mode);

    void setClipboard(const HistoryItem &item, int mode, ClipboardUpdateReason updateReason = ClipboardUpdateReason::UpdateClipboard);
    void moveToTop(int row);

    KSystemClipboard *m_clip = nullptr;

    QList<std::shared_ptr<HistoryItem>> m_items;
    int m_maxSize;
    bool m_displayImages;
    std::recursive_mutex m_mutex;
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
