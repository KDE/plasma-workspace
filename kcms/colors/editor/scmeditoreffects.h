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

#include "ui_scmeditoreffects.h"

class SchemeEditorEffects : public QWidget, public Ui::ScmEditorEffects
{
    Q_OBJECT

public:
    SchemeEditorEffects(const KSharedConfigPtr &config, QPalette::ColorGroup palette, QWidget *parent = nullptr);
    void updateValues();
    void updateFromEffectsPage();

Q_SIGNALS:
    void changed(bool);

private Q_SLOTS:

    void on_intensityBox_currentIndexChanged(int index);

    void on_intensitySlider_valueChanged(int value);

    void on_colorBox_currentIndexChanged(int index);

    void on_colorSlider_valueChanged(int value);

    void on_colorButton_changed(const QColor &color);

    void on_contrastBox_currentIndexChanged(int index);

    void on_contrastSlider_valueChanged(int value);

private:
    QPalette::ColorGroup m_palette;
    KSharedConfigPtr m_config;
    bool m_disableUpdates;
};
