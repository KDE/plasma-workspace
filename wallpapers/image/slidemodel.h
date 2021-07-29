/*
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
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
