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

#include "placeservice.h"
#include "jobs.h"

#include <QDebug>


PlaceService::PlaceService(QObject* parent, KFilePlacesModel* model)
    : Plasma::Service(parent),
      m_model(model)
{
    setName(QStringLiteral("org.kde.places"));

    setDestination(QStringLiteral("places"));
    qDebug() << "Created a place service for" << destination();
}

Plasma::ServiceJob* PlaceService::createJob(const QString& operation,
                                            QMap<QString,QVariant>& parameters)
{
    QModelIndex index = m_model->index(parameters.value(QStringLiteral("placeIndex")).toInt(), 0);

    if (!index.isValid()) {
        return nullptr;
    }

    qDebug() << "Job" << operation << "with arguments" << parameters << "requested";
    if (operation == QLatin1String("Add")) {
        return new AddEditPlaceJob(m_model, index, parameters, this);
    } else if (operation == QLatin1String("Edit")) {
        return new AddEditPlaceJob(m_model, QModelIndex(), parameters, this);
    } else if (operation == QLatin1String("Remove")) {
        return new RemovePlaceJob(m_model, index, this);
    } else if (operation == QLatin1String("Hide")) {
        return new ShowPlaceJob(m_model, index, false, this);
    } else if (operation == QLatin1String("Show")) {
        return new ShowPlaceJob(m_model, index, true, this);
    } else if (operation == QLatin1String("Setup Device")) {
        return new SetupDeviceJob(m_model, index, this);
    } else if (operation == QLatin1String("Teardown Device")) {
        return new TeardownDeviceJob(m_model, index, this);
    } else {
        // FIXME: BAD!  No!
        return nullptr;
    }
}



// vim: sw=4 sts=4 et tw=100
