/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "abstracttasksmodel.h"

#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short An abstract base class for window tasks models.
 *
 * This class serves as abstract base class for window tasks model implementations.
 *
 * It takes care of refreshing the ScreenGeometry data role when screens are added
 * or a screen changes its geometry.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT AbstractWindowTasksModel : public AbstractTasksModel
{
    Q_OBJECT

public:
    explicit AbstractWindowTasksModel(QObject *parent = nullptr);
    ~AbstractWindowTasksModel() override;
};

}
