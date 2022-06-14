/* KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "scmeditoroptions.h"

#include <KConfigGroup>
#include <QDebug>

#include "../colorsapplicator.h"

SchemeEditorOptions::SchemeEditorOptions(KSharedConfigPtr config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
{
    setupUi(this);
    m_disableUpdates = false;
    loadOptions();
}

void SchemeEditorOptions::updateValues()
{
    loadOptions();
}

void SchemeEditorOptions::loadOptions()
{
    KConfigGroup generalGroup(m_config, "General");
    shadeSortedColumn->setChecked(generalGroup.readEntry("shadeSortColumn", true));

    accentTitlebar->setChecked(generalGroup.readEntry("TitlebarIsAccentColored", generalGroup.readEntry("accentActiveTitlebar", false)));

    KConfigGroup KDEgroup(m_config, "KDE");
    contrastSlider->setValue(KDEgroup.readEntry("contrast", KColorScheme::contrastF() * 10));

    KConfigGroup group(m_config, "ColorEffects:Inactive");
    useInactiveEffects->setChecked(group.readEntry("Enable", false));

    tintColors->blockSignals(true);
    tintStrengthSlider->blockSignals(true);

    tintColors->setChecked(generalGroup.hasKey("TintFactor"));
    tintStrengthSlider->setEnabled(generalGroup.hasKey("TintFactor"));
    tintStrengthSlider->setValue(generalGroup.readEntry<qreal>("TintFactor", DefaultTintFactor) * 100);

    tintColors->blockSignals(false);
    tintStrengthSlider->blockSignals(false);

    // NOTE: keep this in sync with kdelibs/kdeui/colors/kcolorscheme.cpp
    // NOTE: remove extra logic from updateFromOptions and on_useInactiveEffects_stateChanged when this changes!
    inactiveSelectionEffect->setChecked(group.readEntry("ChangeSelectionColor", group.readEntry("Enable", true)));
}

// Option slot
void SchemeEditorOptions::on_contrastSlider_valueChanged(int value)
{
    KConfigGroup group(m_config, "KDE");
    group.writeEntry("contrast", value);

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_shadeSortedColumn_stateChanged(int state)
{
    if (m_disableUpdates)
        return;
    KConfigGroup group(m_config, "General");
    group.writeEntry("shadeSortColumn", bool(state != Qt::Unchecked));

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_tintColors_stateChanged(int state)
{
    KConfigGroup group(m_config, "General");
    if (state == Qt::Checked) {
        group.writeEntry("TintFactor", DefaultTintFactor);
        tintStrengthSlider->setEnabled(true);
    } else {
        group.deleteEntry("TintFactor");
        tintStrengthSlider->setEnabled(false);
    }
    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_tintStrengthSlider_valueChanged(int value)
{
    KConfigGroup group(m_config, "General");
    group.writeEntry("TintFactor", (qreal)value / 100.0);

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_useInactiveEffects_stateChanged(int state)
{
    KConfigGroup group(m_config, "ColorEffects:Inactive");
    group.writeEntry("Enable", bool(state != Qt::Unchecked));

    m_disableUpdates = true;
    inactiveSelectionEffect->setChecked(group.readEntry("ChangeSelectionColor", bool(state != Qt::Unchecked)));
    m_disableUpdates = false;

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_inactiveSelectionEffect_stateChanged(int state)
{
    if (m_disableUpdates) {
        // don't write the config as we are reading it!
        return;
    }

    KConfigGroup group(m_config, "ColorEffects:Inactive");
    group.writeEntry("ChangeSelectionColor", bool(state != Qt::Unchecked));

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_accentTitlebar_stateChanged(int state)
{
    if (m_disableUpdates)
        return;

    KConfigGroup group(m_config, "General");
    group.writeEntry("TitlebarIsAccentColored", bool(state == Qt::Checked));
    group.deleteEntry("accentActiveTitlebar");
    group.deleteEntry("accentInactiveTitlebar");

    Q_EMIT changed(true);
}
