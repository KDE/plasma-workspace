/*
 * Copyright 2014 Marco Martin <mart@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef PLACESPROXYMODEL_H
#define PLACESPROXYMODEL_H

#include <KFilePlacesModel>
#include <QIdentityProxyModel>

class PlacesProxyModel : public QIdentityProxyModel
{
    Q_OBJECT

public:
    enum Roles {
        PlaceIndexRole = KFilePlacesModel::CapacityBarRecommendedRole + 100,
        IsDeviceRole,
        PathRole,
        SizeRole,
        UsedRole,
        AvailableRole,
    };

    PlacesProxyModel(QObject *parent, KFilePlacesModel *model);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    KFilePlacesModel *m_placesModel;
};

#endif // PLACESPROXYMODEL_H
