/* KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "scmeditoroptions.h"

#include <KConfigGroup>
#include <QDebug>
#include <QTimer>

#include "../colorsapplicator.h"

using namespace Qt::StringLiterals;

SchemeEditorOptions::SchemeEditorOptions(KSharedConfigPtr config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
    , m_spinboxUpdateTimer(new QTimer(this))
{
    setupUi(this);
    m_disableUpdates = false;
    m_spinboxUpdateTimer->setSingleShot(true);
    // Smoother spinbox
    connect(m_spinboxUpdateTimer, &QTimer::timeout, this, [this]() {
        Q_EMIT changed(true);
        updateContrastExample();
    });
    loadOptions();
    updateContrastExample();
    contrastPercentageSpinBox->setDecimals(0);
}

void SchemeEditorOptions::updateValues()
{
    loadOptions();
}

void SchemeEditorOptions::loadOptions()
{
    KConfigGroup generalGroup(m_config, u"General"_s);
    shadeSortedColumn->setChecked(generalGroup.readEntry("shadeSortColumn", true));

    accentTitlebar->setChecked(generalGroup.readEntry("TitlebarIsAccentColored", generalGroup.readEntry("accentActiveTitlebar", false)));

    KConfigGroup KDEGroup(m_config, u"KDE"_s);
    contrastPercentageSpinBox->blockSignals(true);
    contrastPercentageSpinBox->setValue(KDEGroup.readEntry("frameContrast", KColorScheme::frameContrast()) * 100.0);
    contrastPercentageSpinBox->blockSignals(false);

    KConfigGroup group(m_config, u"ColorEffects:Inactive"_s);
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
void SchemeEditorOptions::on_contrastPercentageSpinBox_valueChanged(double value)
{
    // Convert the old value to something between 1 to 10
    KConfigGroup group(m_config, u"KDE"_s);
    group.writeEntry("Contrast", qRound(value / 10.0));

    // We need to make sure we always write a double
    group.writeEntry("frameContrast", qreal(value / 100.0));

    m_spinboxUpdateTimer->start();
}

void SchemeEditorOptions::on_shadeSortedColumn_stateChanged(int state)
{
    if (m_disableUpdates)
        return;
    KConfigGroup group(m_config, u"General"_s);
    group.writeEntry("shadeSortColumn", bool(state != Qt::Unchecked));

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_tintColors_stateChanged(int state)
{
    KConfigGroup group(m_config, u"General"_s);
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
    KConfigGroup group(m_config, u"General"_s);
    group.writeEntry("TintFactor", (qreal)value / 100.0);

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_useInactiveEffects_stateChanged(int state)
{
    KConfigGroup group(m_config, u"ColorEffects:Inactive"_s);
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

    KConfigGroup group(m_config, u"ColorEffects:Inactive"_s);
    group.writeEntry("ChangeSelectionColor", bool(state != Qt::Unchecked));

    Q_EMIT changed(true);
}

void SchemeEditorOptions::on_accentTitlebar_stateChanged(int state)
{
    if (m_disableUpdates)
        return;

    KConfigGroup group(m_config, u"General"_s);
    group.writeEntry("TitlebarIsAccentColored", bool(state == Qt::Checked));
    group.deleteEntry("accentActiveTitlebar");
    group.deleteEntry("accentInactiveTitlebar");

    Q_EMIT changed(true);
}

void SchemeEditorOptions::updateContrastExample()
{
    const QColor frameColor(
        KColorUtils::mix(palette().color(QPalette::Window), palette().color(QPalette::WindowText), contrastPercentageSpinBox->value() / 100.0));
    contrastExample->setStyleSheet(QStringLiteral("background-color: %1").arg(frameColor.name()));
}

#include "moc_scmeditoroptions.cpp"
