/* KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "scmeditoreffects.h"

#include <KConfigGroup>
#include <QDebug>

SchemeEditorEffects::SchemeEditorEffects(const KSharedConfigPtr &config, QPalette::ColorGroup palette, QWidget *parent)
    : QWidget(parent)
    , m_palette(palette)
    , m_config(config)
{
    setupUi(this);
}

void SchemeEditorEffects::on_intensityBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateFromEffectsPage();
}

void SchemeEditorEffects::on_intensitySlider_valueChanged(int value)
{
    Q_UNUSED(value);
    updateFromEffectsPage();
}

void SchemeEditorEffects::on_colorBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateFromEffectsPage();
}

void SchemeEditorEffects::on_colorSlider_valueChanged(int value)
{
    Q_UNUSED(value);
    updateFromEffectsPage();
}

void SchemeEditorEffects::on_colorButton_changed(const QColor &color)
{
    Q_UNUSED(color);
    updateFromEffectsPage();
}

void SchemeEditorEffects::on_contrastBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateFromEffectsPage();
}

void SchemeEditorEffects::on_contrastSlider_valueChanged(int value)
{
    Q_UNUSED(value);
    updateFromEffectsPage();
}

void SchemeEditorEffects::updateValues()
{
    m_disableUpdates = true;

    // NOTE: keep this in sync with kdelibs/kdeui/colors/kcolorscheme.cpp
    if (m_palette == QPalette::Inactive) {
        KConfigGroup group(m_config, "ColorEffects:Inactive");
        intensityBox->setCurrentIndex(abs(group.readEntry("IntensityEffect", 0)));
        intensitySlider->setValue(int(group.readEntry("IntensityAmount", 0.0) * 20.0) + 20);
        colorBox->setCurrentIndex(abs(group.readEntry("ColorEffect", 2)));
        if (colorBox->currentIndex() > 1) {
            colorSlider->setValue(int(group.readEntry("ColorAmount", 0.025) * 40.0));
        } else {
            colorSlider->setValue(int(group.readEntry("ColorAmount", 0.05) * 20.0) + 20);
        }
        colorButton->setColor(group.readEntry("Color", QColor(112, 111, 110)));
        contrastBox->setCurrentIndex(abs(group.readEntry("ContrastEffect", 2)));
        contrastSlider->setValue(int(group.readEntry("ContrastAmount", 0.1) * 20.0));

    } else if (m_palette == QPalette::Disabled) {
        KConfigGroup group(m_config, "ColorEffects:Disabled");
        intensityBox->setCurrentIndex(group.readEntry("IntensityEffect", 2));
        intensitySlider->setValue(int(group.readEntry("IntensityAmount", 0.1) * 20.0) + 20);
        colorBox->setCurrentIndex(group.readEntry("ColorEffect", 0));
        if (colorBox->currentIndex() > 1) {
            colorSlider->setValue(int(group.readEntry("ColorAmount", 0.0) * 40.0));
        } else {
            colorSlider->setValue(int(group.readEntry("ColorAmount", 0.0) * 20.0) + 20);
        }
        colorButton->setColor(group.readEntry("Color", QColor(56, 56, 56)));
        contrastBox->setCurrentIndex(group.readEntry("ContrastEffect", 1));
        contrastSlider->setValue(int(group.readEntry("ContrastAmount", 0.65) * 20.0));
    } else {
        return;
    }

    m_disableUpdates = false;

    // enable/disable controls
    intensitySlider->setDisabled(intensityBox->currentIndex() == 0);
    colorSlider->setDisabled(colorBox->currentIndex() == 0);
    colorButton->setDisabled(colorBox->currentIndex() < 2);
    contrastSlider->setDisabled(contrastBox->currentIndex() == 0);
    preview->setPalette(m_config, m_palette);
}

void SchemeEditorEffects::updateFromEffectsPage()
{
    if (m_disableUpdates) {
        // don't write the config as we are reading it!
        return;
    }

    QString groupName;
    if (m_palette == QPalette::Inactive) {
        groupName = QStringLiteral("ColorEffects:Inactive");
    } else if (m_palette == QPalette::Disabled) {
        groupName = QStringLiteral("ColorEffects:Disabled");
    } else {
        return;
    }
    KConfigGroup group(m_config, groupName);

    // intensity
    group.writeEntry("IntensityEffect", intensityBox->currentIndex());
    group.writeEntry("IntensityAmount", qreal(intensitySlider->value() - 20) * 0.05);

    // color
    group.writeEntry("ColorEffect", colorBox->currentIndex());
    if (colorBox->currentIndex() > 1) {
        group.writeEntry("ColorAmount", qreal(colorSlider->value()) * 0.025);
    } else {
        group.writeEntry("ColorAmount", qreal(colorSlider->value() - 20) * 0.05);
    }

    group.writeEntry("Color", colorButton->color());

    // contrast
    group.writeEntry("ContrastEffect", contrastBox->currentIndex());
    group.writeEntry("ContrastAmount", qreal(contrastSlider->value()) * 0.05);

    // enable/disable controls
    intensitySlider->setDisabled(intensityBox->currentIndex() == 0);
    colorSlider->setDisabled(colorBox->currentIndex() == 0);
    colorButton->setDisabled(colorBox->currentIndex() < 2);
    contrastSlider->setDisabled(contrastBox->currentIndex() == 0);

    preview->setPalette(m_config, m_palette);

    Q_EMIT changed(true);
}
