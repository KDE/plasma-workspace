/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
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
#include <QSharedPointer>

#include <Plasma/DataEngine>

namespace NotificationManager
{

class Notification;

class JobsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ~JobsModel() override;

    using Ptr = QSharedPointer<JobsModel>;
    static Ptr createJobsModel();

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE void close(const QString &jobId);
    Q_INVOKABLE void expire(const QString &jobId);

    Q_INVOKABLE void suspend(const QString &jobId);
    Q_INVOKABLE void resume(const QString &jobId);
    Q_INVOKABLE void kill(const QString &jobId);

private slots:
    void dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data);

private:
    class Private;
    QScopedPointer<Private> d;

    JobsModel();
    Q_DISABLE_COPY(JobsModel)

};

} // namespace NotificationManager
