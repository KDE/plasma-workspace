/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#pragma once

#include "modeljob.h"

class SetupDeviceJob : public ModelJob
{
    Q_OBJECT

public:
    SetupDeviceJob(KFilePlacesModel *model, QModelIndex index, QObject *parent = nullptr)
        : ModelJob(parent, model, index, QStringLiteral("Setup Device"))
    {
        connect(model, &KFilePlacesModel::setupDone, this, &SetupDeviceJob::setupDone);
        connect(model, &KFilePlacesModel::errorMessage, this, &SetupDeviceJob::setupError);
    }

    void start() override
    {
        m_model->requestSetup(m_index);
    }

private Q_SLOTS:
    void setupDone(const QModelIndex &index, bool success);
    void setupError(const QString &message);
};
