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
#ifndef MODELJOB_H
#define MODELJOB_H

#include <Plasma/ServiceJob>
#include <kfileplacesmodel.h>

class ModelJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    ModelJob(QObject* parent, KFilePlacesModel* model,
             const QModelIndex& index, const QString& operation,
             const QVariantMap& parameters = QVariantMap())
        : ServiceJob(QString::number(index.row()), operation, parameters, parent)
        , m_model(model)
        , m_index(index)
    {}

protected:
    KFilePlacesModel* m_model;
    QModelIndex m_index;
};

#endif // MODELJOB_H

