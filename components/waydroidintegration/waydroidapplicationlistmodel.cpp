/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "waydroidapplicationlistmodel.h"
#include "waydroidapplicationdbusclient.h"
#include "waydroiddbusclient.h"

#include <KLocalizedString>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

WaydroidApplicationListModel::WaydroidApplicationListModel(WaydroidDBusClient *parent)
    : QAbstractListModel{parent}
    , m_waydroidDBusClient{parent}
{
}

WaydroidApplicationListModel::~WaydroidApplicationListModel() = default;

void WaydroidApplicationListModel::initializeApplications(const QList<QDBusObjectPath> &applicationObjectPaths)
{
    if (!m_applications.isEmpty()) {
        return;
    }

    beginResetModel();
    for (const QDBusObjectPath &applicationObjectPath : applicationObjectPaths) {
        auto client = std::make_shared<WaydroidApplicationDBusClient>(applicationObjectPath, this);
        m_applications.append(client);
    }
    endResetModel();
}

void WaydroidApplicationListModel::addApplication(const QDBusObjectPath &objectPath)
{
    beginInsertRows({}, m_applications.size(), m_applications.size());
    auto client = std::make_shared<WaydroidApplicationDBusClient>(objectPath, this);
    connect(client.get(), &WaydroidApplicationDBusClient::nameChanged, this, [this, objectPath] {
        updateApplication(objectPath, {Qt::DisplayRole, DelegateRole, NameRole});
    });
    connect(client.get(), &WaydroidApplicationDBusClient::packageNameChanged, this, [this, objectPath] {
        updateApplication(objectPath, {Qt::DisplayRole, DelegateRole, IdRole});
    });
    m_applications.append(client);
    endInsertRows();
}

void WaydroidApplicationListModel::updateApplication(const QDBusObjectPath &objectPath, const QList<int> &roles)
{
    const auto it = std::ranges::find_if(m_applications, [objectPath](auto app) {
        return app->objectPath() == objectPath;
    });

    if (it == m_applications.end()) {
        return;
    }

    int ind = std::distance(m_applications.begin(), it);
    QModelIndex index = createIndex(ind, 0);
    Q_EMIT dataChanged(index, index, roles);
}

void WaydroidApplicationListModel::removeApplication(const QDBusObjectPath &objectPath)
{
    const auto it = std::ranges::find_if(m_applications, [objectPath](auto app) {
        return app->objectPath() == objectPath;
    });

    if (it == m_applications.end()) {
        return;
    }

    int ind = std::distance(m_applications.begin(), it);
    beginRemoveRows({}, ind, ind);
    m_applications.erase(it);
    endRemoveRows();
}

QHash<int, QByteArray> WaydroidApplicationListModel::roleNames() const
{
    return {{DelegateRole, QByteArrayLiteral("delegate")}, {NameRole, QByteArrayLiteral("name")}, {IdRole, QByteArrayLiteral("id")}};
}

QVariant WaydroidApplicationListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_applications.count()) {
        return QVariant();
    }

    WaydroidApplicationDBusClient::Ptr app = m_applications.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case DelegateRole:
        return QVariant::fromValue(app.get());
    case NameRole:
        return app->name();
    case IdRole:
        return app->packageName();
    default:
        return QVariant();
    }
}

int WaydroidApplicationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_applications.count();
}
