/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#pragma once

#include <Plasma/ServiceJob>
#include <kfileplacesmodel.h>

class ModelJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    ModelJob(QObject *parent, KFilePlacesModel *model, const QModelIndex &index, const QString &operation, const QVariantMap &parameters = QVariantMap())
        : ServiceJob(QString::number(index.row()), operation, parameters, parent)
        , m_model(model)
        , m_index(index)
    {
    }

protected:
    KFilePlacesModel *m_model;
    QModelIndex m_index;
};
