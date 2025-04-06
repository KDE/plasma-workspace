/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2023 Tanbir Jishan <tantalising007@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QAbstractListModel>

class SectionsModel : public QAbstractListModel
{
    Q_OBJECT
    // Constant because we rely on sectionsChanged in AbstractModel
    Q_PROPERTY(int count READ rowCount CONSTANT FINAL)
public:
    SectionsModel(QObject *parent = nullptr);

    enum {
        FirstIndexRole = Qt::UserRole + 1,
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QString lastSection() const;
    // Not using standard QAbstractItemModel functions for these to keep them from being exposed to QML.
    void clear();
    void append(const QString &section, int firstIndex);

private:
    struct Item {
        QString section;
        int firstIndex;
    };

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
};
