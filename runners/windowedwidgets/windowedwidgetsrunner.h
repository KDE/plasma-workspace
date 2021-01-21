/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2010 Marco Martin <notmart@gmail.com>
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

#ifndef WINDOWEDWIDGETSRUNNER_H
#define WINDOWEDWIDGETSRUNNER_H

#include <KService>
#include <QMutex>

#include <krunner/abstractrunner.h>

/**
 * This class looks for matches in the set of .desktop files installed by
 * applications. This way the user can type exactly what they see in the
 * applications menu and have it start the appropriate app. Essentially anything
 * that KService knows about, this runner can launch
 */

class WindowedWidgetsRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    WindowedWidgetsRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~WindowedWidgetsRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

protected Q_SLOTS:
    QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match) override;

private:
    void loadMetadataList();
    QList<KPluginMetaData> m_applets;
    QMutex m_mutex;
};

#endif
