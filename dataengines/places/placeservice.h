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

#ifndef PLACESERVICE_H
#define PLACESERVICE_H

#include <Plasma/Service>

#include "placesengine.h"

class PlaceService : public Plasma::Service
{
    Q_OBJECT

public:
    PlaceService(QObject *parent, KFilePlacesModel *model);

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    KFilePlacesModel *m_model;
    QModelIndex m_index;
};

#endif // PLACESERVICE_H
