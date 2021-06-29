/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

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
