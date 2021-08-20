/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "abstracttasksmodel.h"

namespace TaskManager
{
class TASKMANAGER_EXPORT XStartupTasksModel : public AbstractTasksModel
{
    Q_OBJECT

public:
    explicit XStartupTasksModel(QObject *parent = nullptr);
    ~XStartupTasksModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace TaskManager
