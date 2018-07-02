/*
 *   Copyright 2008 David Edmundson <kde@davidedmundson.co.uk>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#ifndef PLACESRUNNER_H
#define PLACESRUNNER_H


#include <krunner/abstractrunner.h>
#include <kfileplacesmodel.h>

class PlacesRunner;

class PlacesRunnerHelper : public QObject
{
    Q_OBJECT

public:
    explicit PlacesRunnerHelper(PlacesRunner *runner);

public Q_SLOTS:
    void match(Plasma::RunnerContext *context);
    void openDevice(const QString &udi);

private:
    KFilePlacesModel m_places;
    QString m_pendingUdi;
};

class PlacesRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    PlacesRunner(QObject* parent, const QVariantList &args);
    ~PlacesRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;
    QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match) override;

Q_SIGNALS:
    void doMatch(Plasma::RunnerContext *context);

private:
    PlacesRunnerHelper *m_helper;
};


#endif
