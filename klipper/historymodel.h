/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef KLIPPER_HISTORYMODEL_H
#define KLIPPER_HISTORYMODEL_H

#include <QAbstractListModel>
#include <QMutex>

class HistoryItem;

enum class HistoryItemType
{
    Text,
    Image,
    Url
};

class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit HistoryModel(QObject *parent = nullptr);
    ~HistoryModel() override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QHash< int, QByteArray > roleNames() const override;
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

    QMutex *mutex() {
        return &m_mutex;
    }

private:
    void moveToTop(int row);
    QList<QSharedPointer<HistoryItem>> m_items;
    int m_maxSize;
    bool m_displayImages;
    QMutex m_mutex;
};

inline int HistoryModel::maxSize() const
{
    return m_maxSize;
}

inline bool HistoryModel::displayImages() const {
    return m_displayImages;
}

inline void HistoryModel::setDisplayImages(bool show) {
    m_displayImages = show;
}

Q_DECLARE_METATYPE(HistoryItemType)

#endif
