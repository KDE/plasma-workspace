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

#include "ui_scmeditorcolors.h"

class KColorButton;

class SchemeEditorColors : public QWidget, public Ui::ScmEditorColors
{
    Q_OBJECT

public:
    SchemeEditorColors(KSharedConfigPtr config, QWidget *parent = nullptr);
    void updateValues();
    void updateFromColorSchemes();

Q_SIGNALS:
    void changed(bool);

private Q_SLOTS:

    /** slot called when any varies button is clicked */
    void variesClicked();

    /** slot called when color on a KColorButton changes */
    void colorChanged(const QColor &newColor);

    /** set the colortable color buttons up according to the current colorset */
    void updateColorTable();

    /** update m_colorSchemes contents from the values in m_config */
    void updateColorSchemes();

    /** setup the colortable with its buttons and labels */
    void setupColorTable();

private:
    class WindecoColors
    {
    public:
        enum Role { ActiveForeground = 0, ActiveBackground = 1, InactiveForeground = 2, InactiveBackground = 3, ActiveBlend = 4, InactiveBlend = 5 };

        WindecoColors()
        {
        }
        WindecoColors(const KSharedConfigPtr &);
        virtual ~WindecoColors()
        {
        }

        void load(const KSharedConfigPtr &);
        QColor color(Role) const;

    private:
        QColor m_colors[6];
    };

    void changeColor(int row, const QColor &newColor);

    /** helper to create color entries */
    void createColorEntry(const QString &text, const QString &key, QList<KColorButton *> &list, int index);

    void setCommonForeground(KColorScheme::ForegroundRole role, int stackIndex, int buttonIndex);
    void setCommonDecoration(KColorScheme::DecorationRole role, int stackIndex, int buttonIndex);

    /** get the groupKey for the given colorSet */
    static QString colorSetGroupKey(int colorSet);

    QList<KColorButton *> m_backgroundButtons;
    QList<KColorButton *> m_foregroundButtons;
    QList<KColorButton *> m_decorationButtons;
    QList<KColorButton *> m_commonColorButtons;
    QList<KColorScheme> m_colorSchemes;
    QList<QStackedWidget *> m_stackedWidgets;

    QStringList m_colorKeys;

    WindecoColors m_wmColors;

    KSharedConfigPtr m_config;
};
