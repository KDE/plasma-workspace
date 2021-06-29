/* Colorset Preview widget for KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFrame>
#include <QPalette>

#include <KColorScheme>
#include <KSharedConfig>

#include "ui_setpreview.h"

/**
 * The Desktop/Colors tab in kcontrol.
 */
class SetPreviewWidget : public QFrame, Ui::setpreview
{
    Q_OBJECT

public:
    explicit SetPreviewWidget(QWidget *parent);
    ~SetPreviewWidget() override;

    void setPalette(const KSharedConfigPtr &config, KColorScheme::ColorSet);

protected:
    void setPaletteRecursive(QWidget *, const QPalette &);
    bool eventFilter(QObject *, QEvent *) override;
};
