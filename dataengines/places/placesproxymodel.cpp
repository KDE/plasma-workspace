/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "placesproxymodel.h"

#include <QDebug>
#include <QIcon>
#include <QStorageInfo>

PlacesProxyModel::PlacesProxyModel(QObject *parent, KFilePlacesModel *model)
    : QIdentityProxyModel(parent)
    , m_placesModel(model)
{
    setSourceModel(model);
}

QHash<int, QByteArray> PlacesProxyModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(Qt::DisplayRole, "display");
    roles.insert(Qt::DecorationRole, "decoration");
    roles.insert(KFilePlacesModel::UrlRole, "url");
    roles.insert(KFilePlacesModel::HiddenRole, "hidden");
    roles.insert(KFilePlacesModel::SetupNeededRole, "setupNeeded");
    roles.insert(KFilePlacesModel::FixedDeviceRole, "fixedDevice");
    roles.insert(KFilePlacesModel::CapacityBarRecommendedRole, "capacityBarRecommended");
    roles.insert(PlaceIndexRole, "placeIndex");
    roles.insert(IsDeviceRole, "isDevice");
    roles.insert(PathRole, "path");
    roles.insert(SizeRole, "size");
    roles.insert(UsedRole, "used");
    roles.insert(AvailableRole, "available");
    return roles;
}

QVariant PlacesProxyModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case PlaceIndexRole:
        return index.row();
    case IsDeviceRole:
        return m_placesModel->deviceForIndex(index).isValid();
    case PathRole:
        return m_placesModel->url(index).path();

    case SizeRole: {
        const QString path = m_placesModel->url(index).path();
        QStorageInfo info{path};
        return info.isValid() && info.isReady() ? info.bytesTotal() : QVariant{};
    }
    case UsedRole: {
        const QString path = m_placesModel->url(index).path();
        QStorageInfo info{path};
        return info.isValid() && info.isReady() ? info.bytesTotal() - info.bytesAvailable() : QVariant{};
    }
    case AvailableRole: {
        const QString path = m_placesModel->url(index).path();
        QStorageInfo info{path};
        return info.isValid() && info.isReady() ? info.bytesAvailable() : QVariant{};
    }
    default:
        return QIdentityProxyModel::data(index, role);
    }
}
