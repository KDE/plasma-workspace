/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "multipanelview.h"
#include "containmentconfigview.h"
#include "krunner_interface.h"
#include "screenpool.h"
#include "shellcorona.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>
#include <qopenglshaderprogram.h>

#include <PlasmaQuick/AppletQuickItem>

#include <KAuthorized>
#include <KStartupInfo>
#include <KX11Extras>
#include <klocalizedstring.h>
#include <kwindowsystem.h>
#include <plasmaactivities/controller.h>

#include <KPackage/Package>

#include <LayerShellQt/Window>

using namespace Qt::StringLiterals;

MultiPanelView::MultiPanelView(Plasma::Corona *corona, QScreen *targetScreen)
    : PlasmaQuick::QuickViewSharedEngine(nullptr)
{
    QObject::setParent(corona);

    setColor(Qt::transparent);
    // setFlags(Qt::Window | Qt::FramelessWindowHint);

    if (targetScreen) {
        setScreenToFollow(targetScreen);
    }

    rootContext()->setContextProperty(QStringLiteral("desktop"), this);
    setSource(corona->kPackage().fileUrl("views", QStringLiteral("MultiPanel.qml")));
}

MultiPanelView::~MultiPanelView()
{
}

void MultiPanelView::showEvent(QShowEvent *e)
{
    QQuickWindow::showEvent(e);
    adaptToScreen();
}

void MultiPanelView::setScreenToFollow(QScreen *screen)
{
    Q_ASSERT(screen);
    if (screen == m_screenToFollow) {
        return;
    }

    // layer surfaces can't be moved between outputs, so hide and show the window on a new output
    /*  const bool remap = m_layerWindow && isVisible();
      if (remap) {
          setVisible(false);
      }*/

    if (m_screenToFollow) {
        disconnect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &MultiPanelView::screenGeometryChanged);
    }
    m_screenToFollow = screen;
    setScreen(screen);
    connect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &MultiPanelView::screenGeometryChanged);

    // if (remap) {
    setVisible(true);
    // }

    QString rectString;
    QDebug(&rectString) << screen->geometry();
    setTitle(QStringLiteral("%1 @ %2").arg(corona()->kPackage().metadata().name()).arg(rectString));
    adaptToScreen();
    Q_EMIT screenToFollowChanged(screen);
}

QScreen *MultiPanelView::screenToFollow() const
{
    return m_screenToFollow;
}

void MultiPanelView::adaptToScreen()
{
    // This happens sometimes, when shutting down the process
    if (!m_screenToFollow) {
        return;
    }

    screenGeometryChanged();
}

Q_INVOKABLE QString MultiPanelView::fileFromPackage(const QString &key, const QString &fileName)
{
    return corona()->kPackage().filePath(key.toUtf8(), fileName);
}

void MultiPanelView::showConfigurationInterface(Plasma::Applet *applet)
{
}

void MultiPanelView::screenGeometryChanged()
{
    setGeometry(m_screenToFollow->geometry());
    Q_EMIT geometryChanged();
}

void MultiPanelView::addContainemt(Plasma::Containment *containment)
{
    m_containments.append(containment);
    Q_EMIT containmentsChanged();
}

bool MultiPanelView::removeContainment(Plasma::Containment *containment)
{
    return m_containments.removeAll(containment);
    Q_EMIT containmentsChanged();
}

QList<Plasma::Containment *> MultiPanelView::containments() const
{
    return m_containments;
}

QList<QQuickItem *> MultiPanelView::containmentGraphicItems() const
{
    QList<QQuickItem *> list;

    for (auto *containment : std::as_const(m_containments)) {
        QQuickItem *graphicObject = PlasmaQuick::AppletQuickItem::itemForApplet(containment);
        if (graphicObject) {
            list.append(graphicObject);
        }
    }

    return list;
}

Plasma::Corona *MultiPanelView::corona() const
{
    return m_corona;
}

#include "moc_multipanelview.cpp"
