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

#ifndef SM_PLOTTER_HEADER
#define SM_PLOTTER_HEADER

#include <QGraphicsWidget>

#include "sm_export.h"

class QGraphicsLinearLayout;

namespace Plasma {
    class Meter;
    class SignalPlotter;
    class Frame;
}

namespace SM {

class SM_EXPORT Plotter : public QGraphicsWidget
{
        Q_OBJECT
    public:
        Plotter(QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0);
        ~Plotter();

        void addSample(const QList<double>& values);
        void setAnalog(bool analog);
        void setMinMax(double min, double max);
        const QString& title();
        void setTitle(const QString& title);
        void setUnit(const QString& unit);
        void setPlotCount(int count);
        void setCustomPlots(const QList<QColor>& colors);
        void setScale(qreal scale);
        void setStackPlots(bool stack);

    protected Q_SLOTS:
        void themeChanged();

    protected:
        void createWidgets();
        void setOverlayText(const QString& text);
        virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

    private:
        QGraphicsLinearLayout *m_layout;
        Plasma::SignalPlotter *m_plotter;
        Plasma::Meter *m_meter;
        int m_plotCount;
        QString m_title;
        QString m_unit;
        double m_min;
        double m_max;
        Plasma::Frame* m_overlayFrame;
        bool m_showAnalogValue;
};

}

#endif
