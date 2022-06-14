/* ColorEdit widget for KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KColorScheme>
#include <KSharedConfig>

#include <QFrame>
#include <QPalette>
#include <QWidget>

#include "ui_scmeditoroptions.h"

class SchemeEditorOptions : public QWidget, public Ui::ScmEditorOptions
{
    Q_OBJECT

public:
    SchemeEditorOptions(KSharedConfigPtr config, QWidget *parent = nullptr);
    void updateValues();

Q_SIGNALS:
    void changed(bool);

private Q_SLOTS:

    // options slots
    void on_contrastSlider_valueChanged(int value);
    void on_shadeSortedColumn_stateChanged(int state);
    void on_inactiveSelectionEffect_stateChanged(int state);
    void on_useInactiveEffects_stateChanged(int state);
    void on_accentTitlebar_stateChanged(int state);
    void on_tintColors_stateChanged(int state);
    void on_tintStrengthSlider_valueChanged(int value);

private:
    /** load options from global */
    void loadOptions();
    void setCommonForeground(KColorScheme::ForegroundRole role, int stackIndex, int buttonIndex);
    void setCommonDecoration(KColorScheme::DecorationRole role, int stackIndex, int buttonIndex);

    KSharedConfigPtr m_config;
    bool m_disableUpdates;
};
