/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020-2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "ui_killrunner_config.h"
#include <KCModule>

class KillRunnerConfig : public KCModule
{
    Q_OBJECT

public:
    explicit KillRunnerConfig(QObject *parent);

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

private:
    Ui::KillRunnerConfigUi *m_ui;
};
