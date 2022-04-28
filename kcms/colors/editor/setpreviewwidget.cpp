/* Preview widget for KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Urs Wolfer <uwolfer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "setpreviewwidget.h"

void setAutoFill(QWidget *widget)
{
    widget->setAutoFillBackground(true);
    widget->setBackgroundRole(QPalette::Base);
}

SetPreviewWidget::SetPreviewWidget(QWidget *parent)
    : QFrame(parent)
{
    setupUi(this);

    // set correct colors on... lots of things
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);
    setAutoFill(widgetBack0);
    setAutoFill(widgetBack1);
    setAutoFill(widgetBack2);
    setAutoFill(widgetBack3);
    setAutoFill(widgetBack4);
    setAutoFill(widgetBack5);
    setAutoFill(widgetBack6);
    setAutoFill(widgetBack7);
    setAutoFillBackground(true);
    /*
        frame->setBackgroundRole(QPalette::Base);
        viewWidget->setBackgroundRole(QPalette::Base);
        labelView0->setBackgroundRole(QPalette::Base);
        labelView3->setBackgroundRole(QPalette::Base);
        labelView4->setBackgroundRole(QPalette::Base);
        labelView2->setBackgroundRole(QPalette::Base);
        labelView1->setBackgroundRole(QPalette::Base);
        labelView5->setBackgroundRole(QPalette::Base);
        labelView6->setBackgroundRole(QPalette::Base);
        labelView7->setBackgroundRole(QPalette::Base);
        selectionWidget->setBackgroundRole(QPalette::Highlight);
        labelSelection0->setBackgroundRole(QPalette::Highlight);
        labelSelection3->setBackgroundRole(QPalette::Highlight);
        labelSelection4->setBackgroundRole(QPalette::Highlight);
        labelSelection2->setBackgroundRole(QPalette::Highlight);
        labelSelection1->setBackgroundRole(QPalette::Highlight);
        labelSelection5->setBackgroundRole(QPalette::Highlight);
        labelSelection6->setBackgroundRole(QPalette::Highlight);
        labelSelection7->setBackgroundRole(QPalette::Highlight);
    */

    const QList<QWidget *> widgets = findChildren<QWidget *>();
    for (QWidget *widget : widgets) {
        widget->installEventFilter(this);
        widget->setFocusPolicy(Qt::NoFocus);
    }
}

bool SetPreviewWidget::eventFilter(QObject *, QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::Wheel:
    case QEvent::ContextMenu:
        return true; // ignore
    default:
        break;
    }
    return false;
}

void SetPreviewWidget::setPalette(const KSharedConfigPtr &config, KColorScheme::ColorSet set)
{
    QPalette palette = KColorScheme::createApplicationPalette(config);
    KColorScheme::adjustBackground(palette, KColorScheme::NormalBackground, QPalette::Base, set, config);
    QFrame::setPalette(palette);

#define SET_ROLE_PALETTE(n, f, b)                                                                                                                              \
    KColorScheme::adjustForeground(palette, KColorScheme::f, QPalette::Text, set, config);                                                                     \
    labelFore##n->setPalette(palette);                                                                                                                         \
    KColorScheme::adjustBackground(palette, KColorScheme::b, QPalette::Base, set, config);                                                                     \
    labelBack##n->setPalette(palette);                                                                                                                         \
    widgetBack##n->setPalette(palette);

    SET_ROLE_PALETTE(0, NormalText, NormalBackground);
    SET_ROLE_PALETTE(1, InactiveText, AlternateBackground);
    SET_ROLE_PALETTE(2, ActiveText, ActiveBackground);
    SET_ROLE_PALETTE(3, LinkText, LinkBackground);
    SET_ROLE_PALETTE(4, VisitedText, VisitedBackground);
    SET_ROLE_PALETTE(5, NegativeText, NegativeBackground);
    SET_ROLE_PALETTE(6, NeutralText, NeutralBackground);
    SET_ROLE_PALETTE(7, PositiveText, PositiveBackground);

    KColorScheme kcs(QPalette::Active, set, config);
    QBrush deco;

#define SET_DECO_PALETTE(n, d)                                                                                                                                 \
    deco = kcs.decoration(KColorScheme::d);                                                                                                                    \
    palette.setBrush(QPalette::Text, deco);                                                                                                                    \
    labelFore##n->setPalette(palette);

    SET_DECO_PALETTE(8, HoverColor);
    SET_DECO_PALETTE(9, FocusColor);
}
