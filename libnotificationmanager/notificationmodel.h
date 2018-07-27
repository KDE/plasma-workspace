/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QAbstractListModel>
#include <QScopedPointer>

#include "notificationmanager_export.h"

namespace NotificationManager
{

class Notification;

/**
 * @short TODO
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT NotificationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit NotificationModel(QObject *parent = nullptr);
    ~NotificationModel() override;

    enum Roles {
        IdRole = Qt::UserRole + 1,
        CreatedRole = Qt::UserRole + 2,
        SummaryRole = Qt::DisplayRole,
        BodyRole = Qt::UserRole + 3,
        IconNameRole = Qt::UserRole + 4,
        ImageRole = Qt::DecorationRole,
        ApplicationNameRole = Qt::UserRole + 5,
        ApplicationIconNameRole,

        ActionNamesRole,
        ActionLabelsRole,
        HasDefaultActionRole,

        UrlsRole,
        UrgencyRole,
        TimeoutRole,
        PersistentRole,

        IsConfigurableRole,
        ConfigureActionLabelRole,

        ExpiredRole,
        SeenRole,

        // TODO rest
    };
    Q_ENUM(Roles)

    // FIXME currently easier to debug if we crash when accessing invalid role
    // instead of just "randomly" returning display role when
    QVariant data(const QModelIndex &index, int role/* = Qt::DisplayRole*/) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    // TODO should we have an API taking int row, or QModelIndex?
    Q_INVOKABLE void expire(uint notificationId);
    Q_INVOKABLE void dismiss(uint notificationId);
    Q_INVOKABLE void configure(uint notificationId);
    Q_INVOKABLE void invokeDefaultAction(uint notificationId);
    Q_INVOKABLE void invoke(uint notificationId, const QString &actionName);

private:
    class Private;
    QScopedPointer<Private> d;

};

} // namespace NotificationManager
