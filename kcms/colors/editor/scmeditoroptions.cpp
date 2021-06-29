/* KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "scmeditoroptions.h"

#include <KConfigGroup>
#include <QDebug>

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
    KConfigGroup generalGroup(KSharedConfig::openConfig(), "General");
    shadeSortedColumn->setChecked(generalGroup.readEntry("shadeSortColumn", true));

    KConfigGroup KDEgroup(m_config, "KDE");
    contrastSlider->setValue(KDEgroup.readEntry("contrast", KColorScheme::contrast()));

    KConfigGroup group(m_config, "ColorEffects:Inactive");
    useInactiveEffects->setChecked(group.readEntry("Enable", false));

    // NOTE: keep this in sync with kdelibs/kdeui/colors/kcolorscheme.cpp
    // NOTE: remove extra logic from updateFromOptions and on_useInactiveEffects_stateChanged when this changes!
    inactiveSelectionEffect->setChecked(group.readEntry("ChangeSelectionColor", group.readEntry("Enable", true)));
}

// Option slot
void SchemeEditorOptions::on_contrastSlider_valueChanged(int value)
{
    KConfigGroup group(m_config, "KDE");
    group.writeEntry("contrast", value);

    emit changed(true);
}

void SchemeEditorOptions::on_shadeSortedColumn_stateChanged(int state)
{
    if (m_disableUpdates)
        return;
    KConfigGroup group(m_config, "General");
    group.writeEntry("shadeSortColumn", bool(state != Qt::Unchecked));

    emit changed(true);
}

void SchemeEditorOptions::on_useInactiveEffects_stateChanged(int state)
{
    KConfigGroup group(m_config, "ColorEffects:Inactive");
    group.writeEntry("Enable", bool(state != Qt::Unchecked));

    m_disableUpdates = true;
    inactiveSelectionEffect->setChecked(group.readEntry("ChangeSelectionColor", bool(state != Qt::Unchecked)));
    m_disableUpdates = false;

    emit changed(true);
}

void SchemeEditorOptions::on_inactiveSelectionEffect_stateChanged(int state)
{
    if (m_disableUpdates) {
        // don't write the config as we are reading it!
        return;
    }

    KConfigGroup group(m_config, "ColorEffects:Inactive");
    group.writeEntry("ChangeSelectionColor", bool(state != Qt::Unchecked));

    emit changed(true);
}
