/*
 *   Copyright 2008 Sebastian Kügler <sebas@kde.org>
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

#ifndef RECENTDOCUMENTS_H
#define RECENTDOCUMENTS_H

#include <krunner/abstractrunner.h>

#include <QIcon>

class RecentDocuments : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    RecentDocuments(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~RecentDocuments() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;

private Q_SLOTS:
    QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match) override;
};

#endif
