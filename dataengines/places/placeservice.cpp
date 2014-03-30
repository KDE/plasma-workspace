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
    setName("org.kde.places");

    setDestination("places");
    qDebug() << "Created a place service for" << destination();
}

Plasma::ServiceJob* PlaceService::createJob(const QString& operation,
                                            QMap<QString,QVariant>& parameters)
{
    QModelIndex index = m_model->index(parameters.value("index").toInt(), 0);

    if (!index.isValid()) {
        return 0;
    }

    qDebug() << "Job" << operation << "with arguments" << parameters << "requested";
    if (operation == "Add") {
        return new AddEditPlaceJob(m_model, index, parameters, this);
    } else if (operation == "Edit") {
        return new AddEditPlaceJob(m_model, QModelIndex(), parameters, this);
    } else if (operation == "Remove") {
        return new RemovePlaceJob(m_model, index, this);
    } else if (operation == "Hide") {
        return new ShowPlaceJob(m_model, index, false, this);
    } else if (operation == "Show") {
        return new ShowPlaceJob(m_model, index, true, this);
    } else if (operation == "Setup Device") {
        return new SetupDeviceJob(m_model, index, this);
    } else if (operation == "Teardown Device") {
        return new TeardownDeviceJob(m_model, index, this);
    } else {
        // FIXME: BAD!  No!
        return 0;
    }
}

#include "placeservice.moc"

// vim: sw=4 sts=4 et tw=100
