/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2010 Michel Lafon-Puyo <michel.lafonpuyo@gmail.com>
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

#include "applet.h"
#include <math.h>
#include <Plasma/DataEngine>
#include <Plasma/Containment>
#include <Plasma/Frame>
#include <Plasma/IconWidget>
#include <Plasma/ToolTipManager>
#include <KIcon>
#include <QDebug>
#include <QGraphicsLinearLayout>

namespace SM {

Applet::Applet(QObject *parent, const QVariantList &args)
   : Plasma::Applet(parent, args),
     m_interval(10000),
     m_preferredItemHeight(42),
     m_header(0),
     m_engine(0),
     m_orientation(Qt::Vertical),
     m_noSourcesIcon(0),
     m_mode(Desktop),
     m_mainLayout(0),
     m_configSource(0)
{
    if (args.count() > 0 && args[0].toString() == "SM") {
        m_mode = Monitor;
    }

    Plasma::ToolTipManager::self()->registerWidget(this);
}

Applet::~Applet()
{
    removeLayout();
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    if (constraints & Plasma::FormFactorConstraint) {
        if (m_mode == Monitor) {
            setBackgroundHints(NoBackground);
            m_orientation = Qt::Vertical;
        } else {
            SM::Applet::Mode mode = m_mode;
            switch (formFactor()) {
                case Plasma::Planar:
                case Plasma::MediaCenter:
                    mode = Desktop;
                    m_orientation = Qt::Vertical;
                    break;
                case Plasma::Horizontal:
                    mode = Panel;
                    m_orientation = Qt::Horizontal;
                    break;
                case Plasma::Vertical:
                    mode = Panel;
                    m_orientation = Qt::Vertical;
                    break;
            }
            if (mode != m_mode) {
                m_mode = mode;
                connectToEngine();
            }
        }
    } else if (constraints & Plasma::SizeConstraint) {
        checkGeometry();
    }
}

void Applet::setTitle(const QString& title)
{
    m_title = title;
    if (m_header) {
        m_header->setText(m_title);
    }
}

QGraphicsLinearLayout* Applet::mainLayout()
{
   if (!m_mainLayout) {
      m_mainLayout = new QGraphicsLinearLayout(m_orientation);
      m_mainLayout->setContentsMargins(0, 0, 0, 0);
      m_mainLayout->setSpacing(5);
      setLayout(m_mainLayout);
   }
   return m_mainLayout;
}

void Applet::removeLayout()
{
    if (!m_mainLayout) {
        return;
    }

    deleteVisualizations();

    // reset it to no configuration
    // assumes that this only gets called when there's
    // > 0 sources.
    setConfigurationRequired(false);

    delete(m_noSourcesIcon);
    m_noSourcesIcon = 0;

    delete(m_header);
    m_header = 0;

    // We delete the layout since it seems to be only way to remove stretch set for some applets.
    setLayout(0);
    m_mainLayout = 0;
}

void Applet::configureLayout()
{
    mainLayout()->setOrientation(m_orientation);
    if (m_mode != Panel && !m_header) {
        m_header = new Plasma::Frame(this);
        m_header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_header->setText(m_title);
        mainLayout()->addItem(m_header);
    }
}

void Applet::connectToEngine()
{
    removeLayout();
    configureLayout();
    disconnectSources();

    if (m_sources.isEmpty()){
        displayNoAvailableSources();
        constraintsEvent(Plasma::SizeConstraint);
        return;
    }

    foreach (const QString &source, m_sources) {
        if (addVisualization(source)) {
            connectSource(source);
        }
    }

    mainLayout()->activate();
    constraintsEvent(Plasma::SizeConstraint);
}

void Applet::checkGeometry()
{
    QSizeF min;
    QSizeF pref;
    QSizeF max;

    int nb_items = 0;

    nb_items = m_visualizations.count();
    if (nb_items == 0)
        nb_items = 1;

    if (m_mode != Panel) {
        qreal height = 0;
        qreal width = MINIMUM;

        if (m_header) {
            height = m_header->minimumSize().height();
            width = m_header->minimumSize().width();
        }
        min.setHeight(qMax(height + nb_items * MINIMUM,
                             mainLayout()->minimumSize().height()));
        min.setWidth(width + MINIMUM);
        pref.setHeight(height + nb_items * m_preferredItemHeight);
        pref.setWidth(PREFERRED);
        max = QSizeF();
        if (m_mode != Monitor) {
            min += size() - contentsRect().size();
            pref += size() - contentsRect().size();
        } else {
            // Reset margins
            setBackgroundHints(NoBackground);
        }
        //qDebug() << minSize << m_preferredItemHeight << height
        //         << m_minimumHeight << metaObject()->className();

        setAspectRatioMode(Plasma::IgnoreAspectRatio);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        update();
    } else {
        int x = 1;
        int y = 1;
        QSizeF size = containment()->size();
        qreal s;

        if (m_orientation == Qt::Horizontal) {
            x = nb_items;
            s = size.height();
        } else {
            y = nb_items;
            s = size.width();
        }
        min = QSizeF(16 * x, 16 * y);
        max = pref = QSizeF(s * x, s * y);
        setAspectRatioMode(Plasma::KeepAspectRatio);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    setMinimumSize(min);
    setPreferredSize(pref);
    setMaximumSize(max);
    //qDebug() << min << pref << max << metaObject()->className();
    emit geometryChecked();
}

void Applet::connectSource(const QString& source)
{
   if (m_engine) {
      m_engine->connectSource(source, this, m_interval);
      m_connectedSources << source;
   }
}

void Applet::disconnectSources()
{
   if (m_engine) {
      foreach (const QString &source, m_connectedSources) {
         m_engine->disconnectSource(source, this);
      }
   }
   m_connectedSources.clear();
}

void Applet::deleteVisualizations()
{
    if (!m_mainLayout) {
        return;
    }

    foreach (QWeakPointer<QGraphicsWidget> visualization, m_visualizations) {
        delete visualization.data();
    }

    m_visualizations.clear();
    m_toolTips.clear();
}

void Applet::clear()
{
    disconnectSources();
    removeLayout();
    clearSources();
}

void Applet::displayNoAvailableSources()
{
    KIcon appletIcon(icon());
    m_noSourcesIcon = new Plasma::IconWidget(appletIcon, QString(), this);
    mainLayout()->addItem(m_noSourcesIcon);

    m_preferredItemHeight = MINIMUM;

    setConfigurationRequired(true, i18n("No data sources being displayed"));
}

KConfigGroup Applet::config()
{
    if (m_configSource) {
        return m_configSource->config();
    }

    return Plasma::Applet::config();
}

void Applet::save(KConfigGroup &config) const
{
    // work around for saveState being protected
    if (m_mode != Monitor) {
        Plasma::Applet::save(config);
    }
}

void Applet::saveConfig(KConfigGroup &config)
{
    // work around for saveState being protected
    saveState(config);
}

QVariant Applet::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (m_mode == Monitor && change == ItemParentHasChanged) {
        QGraphicsWidget *parent = parentWidget();
        Plasma::Applet *container = 0;
        while (parent) {
            container = qobject_cast<Plasma::Applet *>(parent);

            if (container) {
                break;
            }

            parent = parent->parentWidget();
        }

        if (container && container != containment()) {
            m_configSource = container;
        }
    }

    // We must be able to change position when in monitor even if not mutable
    if (m_mode == Monitor && change == ItemPositionChange) {
        return QGraphicsWidget::itemChange(change, value);
    } else {
        return Plasma::Applet::itemChange(change, value);
    }
}

void Applet::toolTipAboutToShow()
{
    if (mode() == SM::Applet::Panel) {
        if (!m_toolTips.isEmpty()) {
            QString html = "<table>";
            foreach (const QString& s, m_toolTips.values()) {
                if (!s.isEmpty()) {
                    html += s;
                }
            }
            html += "</table>";
            Plasma::ToolTipContent data(title(), html);
            Plasma::ToolTipManager::self()->setContent(this, data);
        } else {
            Plasma::ToolTipManager::self()->clearContent(this);
        }
    }
}

void Applet::appendVisualization(const QString& source, QGraphicsWidget *visualization)
{
    if (m_visualizations.contains(source) && m_visualizations.value(source)) {
        delete(m_visualizations[source].data());
    }
    m_visualizations[source] = visualization;
    mainLayout()->addItem(visualization);
    connect(visualization, SIGNAL(destroyed(QObject*)), this, SLOT(visualizationDestroyed(QObject*)));
}

QGraphicsWidget * Applet::visualization(const QString& source)
{
    return m_visualizations[source].data();
}

void Applet::visualizationDestroyed(QObject *visualization)
{
    QString key;
    QHash<QString, QWeakPointer<QGraphicsWidget> >::const_iterator i;
    for (i = m_visualizations.constBegin(); i != m_visualizations.constEnd(); ++i) {
        if (i.value().data() == static_cast<QGraphicsWidget *>(visualization)) {
            key = i.key();
            break;
        }
    }
    if (!key.isEmpty()) {
        m_visualizations.remove(key);
    }
}

uint Applet::interval()
{
    return m_interval;
}

void Applet::setInterval(uint interval)
{
    m_interval = interval;
}

QString Applet::title()
{
    return m_title;
}

SM::Applet::Mode Applet::mode()
{
    return m_mode;
}

void Applet::setToolTip(const QString &source, const QString &tipContent)
{
    m_toolTips.insert(source, tipContent);
    if (Plasma::ToolTipManager::self()->isVisible(this)) {
        toolTipAboutToShow();
    }
}

void Applet::setEngine(Plasma::DataEngine* engine)
{
    m_engine = engine;
}

Plasma::DataEngine* Applet::engine()
{
    return m_engine;
}

bool Applet::addVisualization(const QString&)
{
    return false;
}

QStringList Applet::connectedSources()
{
    return m_connectedSources;
}

}
