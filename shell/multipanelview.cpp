/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "multipanelview.h"
#include "containmentconfigview.h"
#include "krunner_interface.h"
#include "screenpool.h"
#include "shellcorona.h"

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
#include <kwindoweffects.h>
#include <kwindowsystem.h>
#include <plasmaactivities/controller.h>

#include <KPackage/Package>

#include <LayerShellQt/Window>

#if HAVE_X11
#include <NETWM>
#include <qpa/qplatformwindow_p.h>
#endif

using namespace Qt::StringLiterals;

MultiPanelView::MultiPanelView(Plasma::Corona *corona, QScreen *targetScreen)
    : PlasmaQuick::QuickViewSharedEngine(nullptr)
    , m_corona(corona)
{
    QObject::setParent(corona);

    setColor(Qt::transparent);
    setFlags(Qt::Window | Qt::FramelessWindowHint);
    if (KWindowSystem::isPlatformWayland()) {
        m_layerWindow = LayerShellQt::Window::get(this);
        m_layerWindow->setLayer(LayerShellQt::Window::LayerTop);
        m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        m_layerWindow->setScope(QStringLiteral("dock"));
        m_layerWindow->setCloseOnDismissed(false);
    }

    if (targetScreen) {
        setScreenToFollow(targetScreen);
    }

    // rootContext()->setContextProperty(QStringLiteral("view"), this);
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
    updateLayerWindow();

#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setOnAllDesktops(winId(), true);
        KX11Extras::setType(winId(), NET::Dock);
        if (auto xcbWindow = nativeInterface<QNativeInterface::Private::QXcbWindow>()) {
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Dock);
        }
    }
#endif
}

void MultiPanelView::updateLayerWindow()
{
    if (!m_layerWindow) {
        return;
    }

    m_layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft
                                                            | LayerShellQt::Window::AnchorRight | LayerShellQt::Window::AnchorBottom));
    requestUpdate();
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

void MultiPanelView::addContainment(Plasma::Containment *containment)
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

void MultiPanelView::setMaskFromRectangles(const QList<QRect> &rects)
{
    QRegion region;
    region.setRects(rects);
    setMask(region);
}

void MultiPanelView::setBlurBehindMask(const QList<QObject *> &frameSvgs)
{
    QRegion mask;
    for (auto *obj : frameSvgs) {
        QRegion maskPiece = obj->property("mask").value<QRegion>();
        maskPiece.translate(QPoint(obj->property("x").value<int>(), obj->property("y").value<int>()));
        mask = mask.united(maskPiece);
    }
    // We use mask for graphical effect which tightly  covers the panel
    // For the input region (QWindow::mask) screenPanelRect includes area around the floating
    // panel in order to respect Fitt's law
    KWindowEffects::enableBlurBehind(this, m_theme.blurBehindEnabled(), mask);
    KWindowEffects::enableBackgroundContrast(this,
                                             m_theme.backgroundContrastEnabled(),
                                             m_theme.backgroundContrast(),
                                             m_theme.backgroundIntensity(),
                                             m_theme.backgroundSaturation(),
                                             mask);
}

#include "moc_multipanelview.cpp"
