/*
 *   Copyright (C) 2008 Alex Merry <alex.merry@kdemail.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLACESENGINE_H
#define PLACESENGINE_H

#include <Plasma5Support/DataEngine>

#include <kfileplacesmodel.h>

class PlacesProxyModel;

class PlacesEngine : public Plasma5Support::DataEngine
{
    Q_OBJECT

public:
    PlacesEngine(QObject *parent, const QVariantList &args);
    ~PlacesEngine() override;

    Plasma5Support::Service *serviceForSource(const QString &source) override;

private:
    KFilePlacesModel *m_placesModel;
    PlacesProxyModel *m_proxyModel;
};

#endif // PLACESENGINE_H
