/*
 * Copyright 2008  Alex Merry <alex.merry@kdemail.net>
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
#ifndef SETUPDEVICEJOB_H
#define SETUPDEVICEJOB_H

#include "modeljob.h"

class SetupDeviceJob : public ModelJob
{
    Q_OBJECT

public:
    SetupDeviceJob(KFilePlacesModel* model, QModelIndex index,
                   QObject* parent = nullptr)
        : ModelJob(parent, model, index, QStringLiteral("Setup Device"))
    {
        connect(model, &KFilePlacesModel::setupDone,
                       this, &SetupDeviceJob::setupDone);
        connect(model, &KFilePlacesModel::errorMessage,
                       this, &SetupDeviceJob::setupError);
    }

    void start()
    override {
        m_model->requestSetup(m_index);
    }

private Q_SLOTS:
    void setupDone(const QModelIndex& index, bool success);
    void setupError(const QString& message);
};

#endif // SETUPDEVICEJOB_H
