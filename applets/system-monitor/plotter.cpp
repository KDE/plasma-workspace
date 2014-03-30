/*
 *   Copyright (C) 2010 Petri Damsten <damu@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "plotter.h"
#include <Plasma/SignalPlotter>
#include <Plasma/Meter>
#include <Plasma/Theme>
#include <Plasma/Frame>
#include <KColorUtils>
#include <KGlobalSettings>
#include <QGraphicsLinearLayout>
#include <QWidget>

namespace SM {

Plotter::Plotter(QGraphicsItem* parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
    , m_layout(0)
    , m_plotter(0)
    , m_meter(0)
    , m_plotCount(1)
    , m_min(0.0)
    , m_max(0.0)
    , m_overlayFrame(0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    createWidgets();
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeChanged()));
}

Plotter::~Plotter()
{
}

void Plotter::setAnalog(bool analog)
{
    if (analog && m_layout->count() < 2) {
        m_meter = new Plasma::Meter(this);
        m_meter->setMeterType(Plasma::Meter::AnalogMeter);
        m_meter->setLabelAlignment(1, Qt::AlignCenter);
        m_layout->insertItem(0, m_meter);
        m_meter->setMinimum(m_min);
        m_meter->setMaximum(m_max);
        m_meter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        themeChanged();
    } else if (m_layout->count() > 1) {
        m_layout->removeAt(0);
        delete m_meter;
        m_meter = 0;
    }
}

void Plotter::setMinMax(double min, double max)
{
    if (m_meter) {
        m_meter->setMinimum(min);
        m_meter->setMaximum(max);
    }
    m_plotter->setUseAutoRange(false);
    m_plotter->setVerticalRange(min, max);
    m_min = min;
    m_max = max;
}

const QString& Plotter::title()
{
    return m_title;
}

void Plotter::setTitle(const QString& title)
{
    m_plotter->setTitle(title);
    if (m_meter) {
        m_meter->setLabel(0, title);
    }
    m_title = title;
}

void Plotter::setUnit(const QString& unit)
{
    m_plotter->setUnit(unit);
    m_unit = unit;
}

void Plotter::setScale(qreal scale)
{
    m_plotter->scale(scale);
}

void Plotter::setStackPlots(bool stack)
{
    m_plotter->setStackPlots(stack);
}

void Plotter::setPlotCount(int count)
{
    for (int i = 0; i < m_plotCount; ++i) {
        m_plotter->removePlot(0);
    }
    m_plotCount = count;
    Plasma::Theme* theme = Plasma::Theme::defaultTheme();
    QColor text = theme->color(Plasma::Theme::TextColor);
    QColor bg = theme->color(Plasma::Theme::BackgroundColor);
    for (int i = 0; i < m_plotCount; ++i) {
        QColor color = KColorUtils::tint(text, bg, 0.4 + ((double)i / 2.5));
        m_plotter->addPlot(color);
    }
}

void Plotter::setCustomPlots(const QList<QColor>& colors)
{
    for (int i = 0; i < m_plotCount; ++i) {
        m_plotter->removePlot(0);
    }
    m_plotCount = colors.count();
    foreach (const QColor& color, colors) {
        m_plotter->addPlot(color);
    }
}

void Plotter::createWidgets()
{
    m_layout = new QGraphicsLinearLayout(Qt::Horizontal);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(5);
    setLayout(m_layout);

    m_plotter = new Plasma::SignalPlotter(this);
    m_plotter->setThinFrame(false);
    m_plotter->setShowLabels(false);
    m_plotter->setShowTopBar(true);
    m_plotter->setShowVerticalLines(false);
    m_plotter->setShowHorizontalLines(false);
    m_plotter->setUseAutoRange(true);
    m_plotter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_layout->addItem(m_plotter);
    themeChanged();
    setPlotCount(m_plotCount);
}

void Plotter::themeChanged()
{
    Plasma::Theme* theme = Plasma::Theme::defaultTheme();
    if (m_meter) {
        m_meter->setLabelColor(0, theme->color(Plasma::Theme::TextColor));
        m_meter->setLabelColor(0, theme->color(Plasma::Theme::TextColor));
        m_meter->setLabelColor(1, QColor("#000"));
    }
    m_plotter->setFontColor(theme->color(Plasma::Theme::TextColor));
    m_plotter->setSvgBackground("widgets/plot-background");
    QColor linesColor = theme->color(Plasma::Theme::TextColor);
    linesColor.setAlphaF(0.4);
    m_plotter->setHorizontalLinesColor(linesColor);
    m_plotter->setVerticalLinesColor(linesColor);
    resizeEvent(0);
}

void Plotter::addSample(const QList<double>& values)
{
    m_plotter->addSample(values);
    QStringList list;
    foreach (double value, values) {
        double v = value / m_plotter->scaledBy();
        list << QString("%1 %2").arg(v, 0, 'f', (v > 1000.0) ? 0 : 1).arg(m_unit);
    }
    setOverlayText(list.join(" / "));
    if (m_meter) {
        m_meter->setValue(values[0]);
    }
}

void Plotter::setOverlayText(const QString& text)
{
    if (!m_overlayFrame) {
        QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(Qt::Vertical, m_plotter);
        m_plotter->setLayout(layout);
        m_overlayFrame = new Plasma::Frame(m_plotter);
        m_overlayFrame->setZValue(10);
        m_overlayFrame->resize(m_overlayFrame->size().height() * 2.5,
                               m_overlayFrame->size().height());
        layout->addStretch();
        QGraphicsLinearLayout* layout2 = new QGraphicsLinearLayout(Qt::Horizontal, layout);
        layout2->addStretch();
        layout2->addItem(m_overlayFrame);
        layout2->addStretch();
        layout->addItem(layout2);
        resizeEvent(0);
    }
    m_overlayFrame->setText(text);
    if (m_meter) {
        if (m_showAnalogValue) {
            m_meter->setLabel(1, text);
        } else {
            m_meter->setLabel(1, QString());
        }
    }
}

void Plotter::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    Q_UNUSED(event)
    qreal h = size().height();
    qreal fontHeight = h / (7.0 * 1.5); // Seven rows
    Plasma::Theme* theme = Plasma::Theme::defaultTheme();
    QFont font = theme->font(Plasma::Theme::DefaultFont);
    QFont smallest = KGlobalSettings::smallestReadableFont();
    bool show = false;
    QFontMetrics metrics(font);
    QStringList list;
    for (int i = 0; i < m_plotCount; ++i) {
        list << QString("888.0 %2").arg(m_unit);
    }
    QString valueText = list.join(" / ");

    font.setPointSizeF(smallest.pointSizeF());
    forever {
        metrics = QFontMetrics(font);
        if (metrics.height() > fontHeight) {
            break;
        }
        font.setPointSizeF(font.pointSizeF() + 0.5);
        show = true;
    }
    m_plotter->setFont(font);
    m_plotter->setShowTopBar(metrics.height() < h / 6);
    m_plotter->setShowLabels(show);
    m_plotter->setShowHorizontalLines(show);
    if (m_overlayFrame) {
        m_overlayFrame->setVisible(metrics.height() < h / 3 &&
                                   metrics.width(valueText) < size().width() * 0.8);
        m_overlayFrame->setFont(font);
    }

    if (m_meter) {
        m_meter->setLabelFont(0, font);
        m_meter->setLabelFont(1, font);
        // Make analog meter square
        m_meter->setMinimumSize(h, 8);
        m_showAnalogValue = (m_meter->size().width() * 0.7 > metrics.width(valueText));
        if (m_meter->size().width() * 0.9 > metrics.width(m_title)) {
            m_meter->setLabel(0, m_title);
        } else {
            m_meter->setLabel(0, QString());
        }
        m_meter->setLabel(1, QString());
    }
}

} // namespace

#include "plotter.moc"
