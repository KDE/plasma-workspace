/*
    SPDX-FileCopyrightText: 2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once
#include <QStyle>

namespace KFI
{
class CFontFilterProxyStyle : public QStyle
{
public:
    CFontFilterProxyStyle(QWidget *parent);
    QStyle *style() const;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
    void drawItemPixmap(QPainter *painter, const QRect &rectangle, int alignment, const QPixmap &pixmap) const override;
    void drawItemText(QPainter *painter,
                      const QRect &rectangle,
                      int alignment,
                      const QPalette &palette,
                      bool enabled,
                      const QString &text,
                      QPalette::ColorRole textRole) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *option) const override;
    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option, const QPoint &position, const QWidget *widget) const override;
    QRect itemPixmapRect(const QRect &rectangle, int alignment, const QPixmap &pixmap) const override;
    QRect itemTextRect(const QFontMetrics &metrics, const QRect &rectangle, int alignment, bool enabled, const QString &text) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
    void polish(QWidget *widget) override;
    void polish(QApplication *application) override;
    void polish(QPalette &palette) override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &contentsSize, const QWidget *widget) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const override;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option, const QWidget *widget) const override;
    QPalette standardPalette() const override;
    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const override;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl subControl, const QWidget *widget) const override;
    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override;
    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *application) override;
    int layoutSpacing(QSizePolicy::ControlType control1,
                      QSizePolicy::ControlType control2,
                      Qt::Orientation orientation,
                      const QStyleOption *option = nullptr,
                      const QWidget *widget = nullptr) const override;

protected:
    QWidget *parent;
};

}