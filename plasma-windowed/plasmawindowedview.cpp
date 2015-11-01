/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <QQuickItem>
#include <QResizeEvent>
#include <QQmlExpression>
#include <QMenu>
#include <QQmlEngine>
#include <QQmlContext>

#include <KActionCollection>
#include <KStatusNotifierItem>

#include <Plasma/Package>

#include "plasmawindowedview.h"

PlasmaWindowedView::PlasmaWindowedView(QWindow *parent)
    : QQuickView(parent),
      m_applet(0),
      m_statusNotifier(0),
      m_withStatusNotifier(false)
{
    engine()->rootContext()->setContextProperty(QStringLiteral("root"), contentItem());
    QQmlExpression *expr = new QQmlExpression(engine()->rootContext(), contentItem(), QStringLiteral("Qt.createQmlObject('import QtQuick 2.0; import org.kde.plasma.core 2.0; Rectangle {color: theme.backgroundColor; anchors.fill:parent}', root, \"\");"));
    expr->evaluate();
}

PlasmaWindowedView::~PlasmaWindowedView()
{
}

void PlasmaWindowedView::setHasStatusNotifier(bool stay)
{
    Q_ASSERT(!m_statusNotifier);
    m_withStatusNotifier = stay;
}

void PlasmaWindowedView::setApplet(Plasma::Applet *applet)
{
    m_applet = applet;
    if (!applet) {
        return;
    }

    QQuickItem *i = applet->property("_plasma_graphicObject").value<QQuickItem *>();
    if (!i) {
        return;
    }

    const QRect geom = m_applet->config().readEntry("geometry", QRect());
    if (geom.isValid()) {
        setGeometry(geom);
    }

    i->setParentItem(contentItem());
    i->setVisible(true);
    setTitle(applet->title());
    setIcon(QIcon::fromTheme(applet->icon()));

    foreach (QObject *child, i->children()) {
        //find for the needed property of Layout: minimum/maximum/preferred sizes and fillWidth/fillHeight
        if (child->property("minimumWidth").isValid() && child->property("minimumHeight").isValid() &&
                child->property("preferredWidth").isValid() && child->property("preferredHeight").isValid() &&
                child->property("maximumWidth").isValid() && child->property("maximumHeight").isValid() &&
                child->property("fillWidth").isValid() && child->property("fillHeight").isValid()
           ) {
            m_layout = child;
        }
    }

    if (m_layout) {
        connect(m_layout, SIGNAL(minimumWidthChanged()), this, SLOT(minimumWidthChanged()));
        connect(m_layout, SIGNAL(minimumHeightChanged()), this, SLOT(minimumHeightChanged()));
    }
    QObject::connect(applet->containment(), &Plasma::Containment::configureRequested,
                     this, &PlasmaWindowedView::showConfigurationInterface);

    Q_ASSERT(!m_statusNotifier);
    if (m_withStatusNotifier) {
        m_statusNotifier = new KStatusNotifierItem(this);
        m_statusNotifier->setIconByName(applet->icon());
        m_statusNotifier->setTitle(applet->title());
        m_statusNotifier->setToolTipTitle(applet->title());

        connect(m_statusNotifier, &KStatusNotifierItem::activateRequested, this, [this](bool active, const QPoint& /*pos*/){
            setVisible(active);
            if (active) {
                raise();
            }
        });
    }
}

void PlasmaWindowedView::resizeEvent(QResizeEvent *ev)
{
    if (!m_applet) {
        return;
    }

    QQuickItem *i = m_applet->property("_plasma_graphicObject").value<QQuickItem *>();
    if (!i) {
        return;
    }

    i->setWidth(ev->size().width());
    i->setHeight(ev->size().height());

    contentItem()->setWidth(ev->size().width());
    contentItem()->setHeight(ev->size().height());

    m_applet->config().writeEntry("geometry", QRect(position(), ev->size()));
}

void PlasmaWindowedView::mouseReleaseEvent(QMouseEvent *ev)
{
    QQuickWindow::mouseReleaseEvent(ev);

    if ((!(ev->buttons() & Qt::RightButton) && ev->button() != Qt::RightButton) || ev->isAccepted()) {
        return;
    }

    QMenu menu;

    foreach (QAction *action, m_applet->contextualActions()) {
        if (action) {
            menu.addAction(action);
        }
    }

    if (!m_applet->failedToLaunch()) {
        QAction *runAssociatedApplication = m_applet->actions()->action(QStringLiteral("run associated application"));
        if (runAssociatedApplication && runAssociatedApplication->isEnabled()) {
            menu.addAction(runAssociatedApplication);
        }

        QAction *configureApplet = m_applet->actions()->action(QStringLiteral("configure"));
        if (configureApplet && configureApplet->isEnabled()) {
            menu.addAction(configureApplet);
        }
    }

    menu.exec(ev->globalPos());
    ev->setAccepted(true);
}

void PlasmaWindowedView::moveEvent(QMoveEvent *ev)
{
    Q_UNUSED(ev)
    m_applet->config().writeEntry("geometry", QRect(position(), size()));
}

void PlasmaWindowedView::hideEvent(QHideEvent *ev)
{
    Q_UNUSED(ev)
    m_applet->config().sync();
    if (!m_withStatusNotifier) {
        m_applet->deleteLater();
        deleteLater();
    }
}

void PlasmaWindowedView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_configView) {
        m_configView->hide();
        m_configView->deleteLater();
    }

    if (!applet || !applet->containment()) {
        return;
    }

    m_configView = new PlasmaQuick::ConfigView(applet);

    m_configView->init();
    m_configView->show();
}

void PlasmaWindowedView::minimumWidthChanged()
{
    if (!m_layout) {
        return;
    }

    setMinimumWidth(m_layout->property("minimumWidth").toInt());
}

void PlasmaWindowedView::minimumHeightChanged()
{
    if (!m_layout) {
        return;
    }

    setMinimumHeight(m_layout->property("minimumHeight").toInt());
}

void PlasmaWindowedView::maximumWidthChanged()
{
    if (!m_layout) {
        return;
    }

    setMaximumWidth(m_layout->property("maximumWidth").toInt());
}

void PlasmaWindowedView::maximumHeightChanged()
{
    if (!m_layout) {
        return;
    }

    setMaximumHeight(m_layout->property("maximumHeight").toInt());
}

