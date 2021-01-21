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

#include "plasmawindowedview.h"

#include <QMenu>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQmlProperty>
#include <QQuickItem>
#include <QResizeEvent>

#include <KActionCollection>
#include <KIconLoader>
#include <KLocalizedString>
#include <KStatusNotifierItem>

#include <Plasma/Package>

PlasmaWindowedView::PlasmaWindowedView(QWindow *parent)
    : QQuickView(parent)
    , m_applet(nullptr)
    , m_statusNotifier(nullptr)
    , m_withStatusNotifier(false)
{
    engine()->rootContext()->setContextProperty(QStringLiteral("root"), contentItem());
    // access appletInterface.Layout.minimumWidth, to create the Layout attached object for appletInterface as a sideeffect
    QQmlExpression *expr = new QQmlExpression(
        engine()->rootContext(),
        contentItem(),
        QStringLiteral(
            "Qt.createQmlObject('import QtQuick 2.0; import QtQuick.Layouts 1.1; import org.kde.plasma.core 2.0; Rectangle {color: theme.backgroundColor; "
            "anchors.fill:parent; property Item appletInterface; onAppletInterfaceChanged: print(appletInterface.Layout.minimumWidth)}', root, \"\");"));
    m_rootObject = expr->evaluate().value<QQuickItem *>();
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

    m_appletInterface = applet->property("_plasma_graphicObject").value<QQuickItem *>();

    if (!m_appletInterface) {
        return;
    }

    m_appletInterface->setParentItem(m_rootObject);
    m_rootObject->setProperty("appletInterface", QVariant::fromValue(m_appletInterface.data()));
    m_appletInterface->setVisible(true);
    setTitle(applet->title());
    setIcon(QIcon::fromTheme(applet->icon()));

    const QSize switchSize(m_appletInterface->property("switchWidth").toInt(), m_appletInterface->property("switchHeight").toInt());
    QRect geom = m_applet->config().readEntry("geometry", QRect());

    if (geom.isValid()) {
        geom.setWidth(qMax(geom.width(), switchSize.width() + 1));
        geom.setHeight(qMax(geom.height(), switchSize.height() + 1));
        setGeometry(geom);
    }
    setMinimumSize(QSize(qMax((int)KIconLoader::SizeEnormous, switchSize.width() + 1), qMax((int)KIconLoader::SizeEnormous, switchSize.height() + 1)));

    foreach (QObject *child, m_appletInterface->children()) {
        // find for the needed property of Layout: minimum/maximum/preferred sizes and fillWidth/fillHeight
        if (child->property("minimumWidth").isValid() && child->property("minimumHeight").isValid() && child->property("preferredWidth").isValid()
            && child->property("preferredHeight").isValid() && child->property("maximumWidth").isValid() && child->property("maximumHeight").isValid()
            && child->property("fillWidth").isValid() && child->property("fillHeight").isValid()) {
            m_layout = child;
        }
    }

    if (m_layout) {
        connect(m_layout, SIGNAL(minimumWidthChanged()), this, SLOT(minimumWidthChanged()));
        connect(m_layout, SIGNAL(minimumHeightChanged()), this, SLOT(minimumHeightChanged()));
    }
    minimumWidthChanged();
    minimumHeightChanged();
    QObject::connect(applet->containment(), &Plasma::Containment::configureRequested, this, &PlasmaWindowedView::showConfigurationInterface);

    Q_ASSERT(!m_statusNotifier);
    if (m_withStatusNotifier) {
        m_statusNotifier = new KStatusNotifierItem(applet->pluginMetaData().pluginId(), this);
        m_statusNotifier->setStandardActionsEnabled(false); // we add our own "Close" entry manually below

        updateSniIcon();
        connect(applet, &Plasma::Applet::iconChanged, this, &PlasmaWindowedView::updateSniIcon);

        updateSniTitle();
        connect(applet, &Plasma::Applet::titleChanged, this, &PlasmaWindowedView::updateSniTitle);

        updateSniStatus();
        connect(applet, &Plasma::Applet::statusChanged, this, &PlasmaWindowedView::updateSniStatus);

        // set up actions
        for (auto a : applet->contextualActions()) {
            m_statusNotifier->contextMenu()->addAction(a);
        }
        m_statusNotifier->contextMenu()->addSeparator();
        QAction *closeAction = new QAction(QIcon::fromTheme(QStringLiteral("window-close")), i18n("Close %1", applet->title()), this);
        connect(closeAction, &QAction::triggered, this, [this]() {
            m_statusNotifier->deleteLater();
            close();
        });
        m_statusNotifier->contextMenu()->addAction(closeAction);

        connect(m_statusNotifier.data(), &KStatusNotifierItem::activateRequested, this, [this](bool /*active*/, const QPoint & /*pos*/) {
            if (isVisible() && isActive()) {
                hide();
            } else {
                show();
                raise();
            }
        });
        auto syncStatus = [this]() {
            switch (m_applet->status()) {
            case Plasma::Types::AcceptingInputStatus:
            case Plasma::Types::RequiresAttentionStatus:
            case Plasma::Types::NeedsAttentionStatus:
                m_statusNotifier->setStatus(KStatusNotifierItem::NeedsAttention);
                break;
            case Plasma::Types::ActiveStatus:
                m_statusNotifier->setStatus(KStatusNotifierItem::Active);
                break;
            default:
                m_statusNotifier->setStatus(KStatusNotifierItem::Passive);
            }
        };
        connect(applet, &Plasma::Applet::statusChanged, this, syncStatus);
        syncStatus();
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

    minimumWidthChanged();
    minimumHeightChanged();

    i->setSize(ev->size());
    contentItem()->setSize(ev->size());

    m_applet->config().writeEntry("geometry", QRect(position(), ev->size()));
}

void PlasmaWindowedView::mouseReleaseEvent(QMouseEvent *ev)
{
    QQuickWindow::mouseReleaseEvent(ev);

    if ((!(ev->buttons() & Qt::RightButton) && ev->button() != Qt::RightButton) || ev->isAccepted()) {
        return;
    }

    emit m_applet->contextualActionsAboutToShow();

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

void PlasmaWindowedView::keyPressEvent(QKeyEvent *ev)
{
    if (ev->matches(QKeySequence::Quit)) {
        m_statusNotifier->deleteLater();
        close();
    }
    QQuickView::keyReleaseEvent(ev);
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
    if (!m_layout || !m_appletInterface) {
        return;
    }

    setMinimumWidth(
        qMax(m_appletInterface->property("switchWidth").toInt() + 1, qMax((int)KIconLoader::SizeEnormous, m_layout->property("minimumWidth").toInt())));
}

void PlasmaWindowedView::minimumHeightChanged()
{
    if (!m_layout || !m_appletInterface) {
        return;
    }

    setMinimumHeight(
        qMax(m_appletInterface->property("switchHeight").toInt() + 1, qMax((int)KIconLoader::SizeEnormous, m_layout->property("minimumHeight").toInt())));
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

void PlasmaWindowedView::updateSniIcon()
{
    m_statusNotifier->setIconByName(m_applet->icon());
}

void PlasmaWindowedView::updateSniTitle()
{
    m_statusNotifier->setTitle(m_applet->title());
    m_statusNotifier->setToolTipTitle(m_applet->title());
}

void PlasmaWindowedView::updateSniStatus()
{
    switch (m_applet->status()) {
    case Plasma::Types::UnknownStatus:
    case Plasma::Types::PassiveStatus:
    case Plasma::Types::HiddenStatus:
        m_statusNotifier->setStatus(KStatusNotifierItem::Passive);
        break;
    case Plasma::Types::ActiveStatus:
    case Plasma::Types::AcceptingInputStatus:
        m_statusNotifier->setStatus(KStatusNotifierItem::Active);
        break;
    case Plasma::Types::NeedsAttentionStatus:
    case Plasma::Types::RequiresAttentionStatus:
        m_statusNotifier->setStatus(KStatusNotifierItem::NeedsAttention);
        break;
    }
}
