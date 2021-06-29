/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef PLACESENGINE_H
#define PLACESENGINE_H

#include <Plasma/DataEngine>

#include <kfileplacesmodel.h>

class PlacesProxyModel;

class PlacesEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    PlacesEngine(QObject *parent, const QVariantList &args);
    ~PlacesEngine() override;

    Plasma::Service *serviceForSource(const QString &source) override;

private:
    KFilePlacesModel *m_placesModel;
    PlacesProxyModel *m_proxyModel;
};

#endif // PLACESENGINE_H
