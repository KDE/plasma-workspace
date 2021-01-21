/*
 *   Copyright (C) 2007 Aaron Seigo <aseigo@kde.org>
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

#ifndef LOCATIONRUNNER_H
#define LOCATIONRUNNER_H

#include <krunner/abstractrunner.h>

class LocationsRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    LocationsRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~LocationsRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

protected Q_SLOTS:
    QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match) override;
};

#endif
