/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
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

#ifndef SERVICERUNNER_H
#define SERVICERUNNER_H


#include <KService>

//#include <KRunner/AbstractRunner>
#include <krunner/abstractrunner.h>

/**
 * This class looks for matches in the set of .desktop files installed by
 * applications. This way the user can type exactly what they see in the
 * applications menu and have it start the appropriate app. Essentially anything
 * that KService knows about, this runner can launch
 */

class ServiceRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

    public:
        ServiceRunner(QObject *parent, const QVariantList &args);
        ~ServiceRunner() override;

        void match(Plasma::RunnerContext &context) override;
        void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;
        QStringList categories() const override;
        QIcon categoryIcon(const QString& category) const override;

    protected Q_SLOTS:
        QMimeData * mimeDataForMatch(const Plasma::QueryMatch &match) override;

    protected:
        void setupMatch(const KService::Ptr &service, Plasma::QueryMatch &action);
};


#endif

