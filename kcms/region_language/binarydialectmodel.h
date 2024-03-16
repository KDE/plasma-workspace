/*
    binarydialectmodel.h
    SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "qabstractitemmodel.h"

class BinaryDialectModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum RoleName { DisplayName = Qt::DisplayRole, Example, Description };
    explicit BinaryDialectModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};
