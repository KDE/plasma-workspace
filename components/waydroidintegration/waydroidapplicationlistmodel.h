/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "waydroidapplicationdbusclient.h"

#include <QAbstractListModel>
#include <QObject>
#include <QTimer>

class WaydroidDBusClient;

class WaydroidApplicationListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        DelegateRole = Qt::UserRole + 1,
        NameRole,
        IdRole
    };

    explicit WaydroidApplicationListModel(WaydroidDBusClient *parent = nullptr);
    ~WaydroidApplicationListModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void initializeApplications(const QList<QDBusObjectPath> &applicationObjectPaths);

public Q_SLOTS:
    void addApplication(const QDBusObjectPath &objectPath);
    void removeApplication(const QDBusObjectPath &objectPath);

private:
    WaydroidDBusClient *m_waydroidDBusClient{nullptr};
    QList<WaydroidApplicationDBusClient::Ptr> m_applications;
    QTimer *m_refreshTimer{nullptr};

    void updateApplication(const QDBusObjectPath &objectPath, const QList<int> &roles);
};