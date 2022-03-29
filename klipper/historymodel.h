/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QAbstractListModel>
#include <QRecursiveMutex>

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

    void insert(QSharedPointer<HistoryItem> item);
    void clearAndBatchInsert(const QVector<QSharedPointer<HistoryItem>> &items);

    QRecursiveMutex *mutex()
    {
        return &m_mutex;
    }

private:
    void moveToTop(int row);
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
