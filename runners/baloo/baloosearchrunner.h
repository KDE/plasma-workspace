/*
 * This file is part of the KDE Baloo Project
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef _BALOO_SEARCH_RUNNER_H_
#define _BALOO_SEARCH_RUNNER_H_

#include <KRunner/AbstractRunner>
#include <KRunner/QueryMatch>

class SearchRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    SearchRunner(QObject* parent, const QVariantList& args);
    SearchRunner(QObject* parent, const QString& serviceId = QString());
    ~SearchRunner();

    void match(Plasma::RunnerContext& context);
    void run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& action);

    virtual QStringList categories() const;
    virtual QIcon categoryIcon(const QString& category) const;

protected Q_SLOTS:
    void init();

private:
    void match(Plasma::RunnerContext& context, const QString& type,
               const QString& category);
};

#endif // _BALOO_SEARCH_RUNNER_H_
