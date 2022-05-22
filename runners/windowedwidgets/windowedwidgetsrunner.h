/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2010 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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
