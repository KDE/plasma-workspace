/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "splashwindow.h"
#include "debug.h"
#include "splashapp.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQmlContext>
#include <QQuickItem>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QTimer>

#include <LayerShellQt/Window>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KWindowSystem>

using namespace Qt::StringLiterals;

SplashWindow::SplashWindow(bool testing, bool window, const QString &theme, QScreen *screen)
    : PlasmaQuick::QuickViewSharedEngine()
    , m_stage(0)
    , m_testing(testing)
    , m_window(window)
    , m_theme(theme)
{
    if (KWindowSystem::isPlatformWayland()) {
        if (auto layerShellWindow = LayerShellQt::Window::get(this)) {
            layerShellWindow->setScope(QStringLiteral("ksplashqml"));
            layerShellWindow->setLayer(LayerShellQt::Window::LayerOverlay);
            layerShellWindow->setExclusiveZone(-1);
            layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
        }
    }

    setCursor(Qt::BlankCursor);
    setScreen(screen);
    setColor(Qt::transparent);
    setDefaultAlphaBuffer(true);
    setResizeMode(PlasmaQuick::QuickViewSharedEngine::SizeRootObjectToView);

    if (!m_window) {
        setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    }

    if (!m_testing && !m_window) {
        if (KWindowSystem::isPlatformX11()) {
            // X11 specific hint only on X11
            setFlags(Qt::BypassWindowManagerHint);
        } else if (!KWindowSystem::isPlatformWayland()) {
            // on other platforms go fullscreen
            // on Wayland we cannot go fullscreen due to QTBUG 54883
            setWindowState(Qt::WindowFullScreen);
        }
    }

    if (m_testing && !m_window && !KWindowSystem::isPlatformWayland()) {
        setWindowState(Qt::WindowFullScreen);
    }

    // be sure it will be eventually closed
    // FIXME: should never be stuck
    QTimer::singleShot(30000, this, &QWindow::close);
}

void SplashWindow::setStage(int stage)
{
    m_stage = stage;
    Q_ASSERT(rootObject());
    if (rootObject()) { // could be null if source loading failed
        rootObject()->setProperty("stage", stage);
    }
}

void SplashWindow::keyPressEvent(QKeyEvent *event)
{
    PlasmaQuick::QuickViewSharedEngine::keyPressEvent(event);
    if (m_testing && !event->isAccepted() && event->key() == Qt::Key_Escape) {
        close();
    }
}

void SplashWindow::mousePressEvent(QMouseEvent *event)
{
    PlasmaQuick::QuickViewSharedEngine::mousePressEvent(event);
    if (m_testing && !event->isAccepted()) {
        close();
    }
}

void SplashWindow::setGeometry(const QRect &rect)
{
    bool oldGeometryEmpty = geometry().isNull();
    PlasmaQuick::QuickViewSharedEngine::setGeometry(rect);

    if (oldGeometryEmpty) {
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
        const auto originalPackagePath = package.path();
        KConfigGroup cg(KSharedConfig::openConfig(), u"KDE"_s);
        const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
        if (!packageName.isEmpty()) {
            package.setPath(packageName);
        }

        if (!m_theme.isEmpty()) {
            package.setPath(m_theme);
        }

        const auto url = QUrl::fromLocalFile(package.filePath("splashmainscript"));
        qCDebug(KSPLASHQML_DEBUG) << "Loading" << url << "from" << package.path();
        if (!package.isValid() || !url.isValid() || url.isEmpty()) {
            qCWarning(KSPLASHQML_DEBUG) << "Failed to resolve package url" << url //
                                        << "package.valid" << package.isValid() //
                                        << "package.path" << package.path() //
                                        << "originalPackagePath" << originalPackagePath //
                                        << "packageName" << packageName //
                                        << "theme" << m_theme;
            Q_ASSERT(package.isValid());
            Q_ASSERT(url.isValid());
            Q_ASSERT(!url.isEmpty());
        }
        setSource(url);
    }
}
