/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "backgroundlistmodel.h"

class SlideModel : public BackgroundListModel
{
    Q_OBJECT
public:
    using BackgroundListModel::BackgroundListModel;
    void reload(const QStringList &selected);
    void addDirs(const QStringList &selected);
    void removeDir(const QString &selected);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void done();

private Q_SLOTS:
    void removeBackgrounds(const QStringList &paths, const QString &token);
    void backgroundsFound(const QStringList &paths, const QString &token);
};
