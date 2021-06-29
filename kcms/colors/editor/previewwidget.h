/* Preview widget for KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFrame>
#include <QPalette>

#include <KSharedConfig>

#include "ui_preview.h"

/**
 * The Desktop/Colors tab in kcontrol.
 */
class PreviewWidget : public QFrame, Ui::preview
{
    Q_OBJECT

public:
    PreviewWidget(QWidget *parent);
    ~PreviewWidget() override;

    void setPalette(const KSharedConfigPtr &config, QPalette::ColorGroup state = QPalette::Active);

protected:
    void setPaletteRecursive(QWidget *, const QPalette &);
    bool eventFilter(QObject *, QEvent *) override;
};
