/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QAbstractListModel>
#include <QRecursiveMutex>

class HistoryItem;

enum class HistoryItemType {
    Text,
    Image,
    Url,
};

class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool count READ rowCount NOTIFY countChanged)

public:
    enum RoleType {
        HistoryItemConstPtrRole = Qt::UserRole,
        UuidRole,
        TypeRole,
        Base64UuidRole,
        TypeIntRole,
        FullTextRole,
        PinnedSortRole
    };
    Q_ENUM(RoleType)

    explicit HistoryModel(QObject *parent = nullptr);
    ~HistoryModel() override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent, bool removeAll);
    QHash<int, QByteArray> roleNames() const override;
    bool remove(const QByteArray &uuid);

    bool pinned(const QByteArray &uuid);
    void togglePin(const QByteArray &uuid);
    QList<QByteArray> pinnedUuids() const;
    int countUnpinned();

    int maxSize() const;
    void setMaxSize(int size);

    bool displayImages() const;
    void setDisplayImages(bool show);

    void clear(bool clearAll = false);
    void moveToTop(const QByteArray &uuid);
    void moveTopToBack();
    void moveBackToTop();

    QModelIndex indexOf(const QByteArray &uuid) const;
    QModelIndex indexOf(const HistoryItem *item) const;

    void insert(QSharedPointer<HistoryItem> item, bool force = false);
    void forceInsert(QSharedPointer<HistoryItem> item);

    QRecursiveMutex *mutex()
    {
        return &m_mutex;
    }
signals:
    void countChanged(int);

private:
    void moveToTop(int row);
    // keep the pinned time order of pinned items
    QList<QByteArray> m_pinnedUuids;
    QList<QSharedPointer<HistoryItem>> m_items;
    int m_maxSize;
    bool m_displayImages;
    QRecursiveMutex m_mutex;
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

Q_DECLARE_METATYPE(HistoryItemType)
