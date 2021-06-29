/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

// Project-Includes
#include "ui_killrunner_config.h"
// KDE-Includes
#include <KCModule>
// Qt

class KillRunnerConfigForm : public QWidget, public Ui::KillRunnerConfigUi
{
    Q_OBJECT

public:
    explicit KillRunnerConfigForm(QWidget *parent);
};

class KillRunnerConfig : public KCModule
{
    Q_OBJECT

public:
    explicit KillRunnerConfig(QWidget *parent = nullptr, const QVariantList &args = QVariantList());

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

private:
    KillRunnerConfigForm *m_ui;
};