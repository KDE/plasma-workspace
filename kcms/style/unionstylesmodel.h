// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

#pragma once

#include <memory>

#include <QAbstractListModel>

#include <Union/StylePackage.h>

class UnionStylesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    UnionStylesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE int indexOf(const QString &styleId);
    Q_INVOKABLE void refresh();

private:
    QList<Union::StylePackage> m_styles;
};
