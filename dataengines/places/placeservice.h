/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <Plasma5Support/Service>

#include "placesengine.h"

class PlaceService : public Plasma5Support::Service
{
    Q_OBJECT

public:
    PlaceService(QObject *parent, KFilePlacesModel *model);

protected:
    Plasma5Support::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    KFilePlacesModel *m_model;
    QModelIndex m_index;
};
