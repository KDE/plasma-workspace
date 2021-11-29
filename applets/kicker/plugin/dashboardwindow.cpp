/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dashboardwindow.h"

#include <QCoreApplication>
#include <QIcon>
#include <QScreen>

#include <KWindowEffects>
#include <KWindowSystem>

DashboardWindow::DashboardWindow(QQuickItem *parent)
    : QQuickWindow(parent ? parent->window() : nullptr)
    , m_mainItem(nullptr)
    , m_visualParentItem(nullptr)
    , m_visualParentWindow(nullptr)
{
    setClearBeforeRendering(true);
    setFlags(Qt::FramelessWindowHint);

    setIcon(QIcon::fromTheme(QStringLiteral("plasma")));

    connect(&m_theme, &Plasma::Theme::themeChanged, this, &DashboardWindow::updateTheme);
}

DashboardWindow::~DashboardWindow()
{
}

QQuickItem *DashboardWindow::mainItem() const
{
    return m_mainItem;
}

void DashboardWindow::setMainItem(QQuickItem *item)
{
    if (m_mainItem != item) {
        if (m_mainItem) {
            m_mainItem->setVisible(false);
        }

        m_mainItem = item;

        if (m_mainItem) {
            m_mainItem->setVisible(isVisible());
            m_mainItem->setParentItem(contentItem());
        }

        Q_EMIT mainItemChanged();
    }
}

QQuickItem *DashboardWindow::visualParent() const
{
    return m_visualParentItem;
}

void DashboardWindow::setVisualParent(QQuickItem *item)
{
    if (m_visualParentItem != item) {
        if (m_visualParentItem) {
            disconnect(m_visualParentItem.data(), &QQuickItem::windowChanged, this, &DashboardWindow::visualParentWindowChanged);
        }

        m_visualParentItem = item;

        if (m_visualParentItem) {
            if (m_visualParentItem->window()) {
                visualParentWindowChanged(m_visualParentItem->window());
            }

            connect(m_visualParentItem.data(), &QQuickItem::windowChanged, this, &DashboardWindow::visualParentWindowChanged);
        }

        Q_EMIT visualParentChanged();
    }
}

QColor DashboardWindow::backgroundColor() const
{
    return color();
}

void DashboardWindow::setBackgroundColor(const QColor &c)
{
    if (color() != c) {
        setColor(c);

        Q_EMIT backgroundColorChanged();
    }
}

QQuickItem *DashboardWindow::keyEventProxy() const
{
    return m_keyEventProxy;
}

void DashboardWindow::setKeyEventProxy(QQuickItem *item)
{
    if (m_keyEventProxy != item) {
        m_keyEventProxy = item;

        Q_EMIT keyEventProxyChanged();
    }
}

void DashboardWindow::toggle()
{
    if (isVisible()) {
        close();
    } else {
        resize(screen()->size());
        showFullScreen();
        KWindowSystem::forceActiveWindow(winId());
    }
}

bool DashboardWindow::event(QEvent *event)
{
    if (event->type() == QEvent::Expose) {
        // FIXME TODO: We can remove this once we depend on Qt 5.6.1+.
        // See: https://bugreports.qt.io/browse/QTBUG-26978
        KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
    } else if (event->type() == QEvent::PlatformSurface) {
        const QPlatformSurfaceEvent *pSEvent = static_cast<QPlatformSurfaceEvent *>(event);

        if (pSEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
        }
    } else if (event->type() == QEvent::Show) {
        updateTheme();

        if (m_mainItem) {
            m_mainItem->setVisible(true);
        }
    } else if (event->type() == QEvent::Hide) {
        if (m_mainItem) {
            m_mainItem->setVisible(false);
        }
    } else if (event->type() == QEvent::FocusOut) {
        if (isVisible()) {
            KWindowSystem::raiseWindow(winId());
            KWindowSystem::forceActiveWindow(winId());
        }
    }

    return QQuickWindow::event(event);
}

void DashboardWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        Q_EMIT keyEscapePressed();

        return;
        // clang-format off
    } else if (m_keyEventProxy && !m_keyEventProxy->hasActiveFocus()
        && !(e->key() == Qt::Key_Home)
        && !(e->key() == Qt::Key_End)
        && !(e->key() == Qt::Key_Left)
        && !(e->key() == Qt::Key_Up)
        && !(e->key() == Qt::Key_Right)
        && !(e->key() == Qt::Key_Down)
        && !(e->key() == Qt::Key_PageUp)
        && !(e->key() == Qt::Key_PageDown)
        && !(e->key() == Qt::Key_Enter)
        && !(e->key() == Qt::Key_Return)
        && !(e->key() == Qt::Key_Menu)
        && !(e->key() == Qt::Key_Tab)
        && !(e->key() == Qt::Key_Backtab)) {
        // clang-format on
        QPointer<QQuickItem> previousFocusItem = activeFocusItem();

        m_keyEventProxy->forceActiveFocus();
        QEvent *eventCopy = new QKeyEvent(e->type(),
                                          e->key(),
                                          e->modifiers(),
                                          e->nativeScanCode(),
                                          e->nativeVirtualKey(),
                                          e->nativeModifiers(),
                                          e->text(),
                                          e->isAutoRepeat(),
                                          e->count());
        QCoreApplication::postEvent(this, eventCopy);

        // We _need_ to do it twice to make sure the event ping-pong needed
        // for delivery happens before we sap focus again.
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();

        if (previousFocusItem) {
            previousFocusItem->forceActiveFocus();
        }

        return;
    }

    QQuickWindow::keyPressEvent(e);
}

void DashboardWindow::updateTheme()
{
    KWindowEffects::enableBlurBehind(this, true);
}

void DashboardWindow::visualParentWindowChanged(QQuickWindow *window)
{
    if (m_visualParentWindow) {
        disconnect(m_visualParentWindow.data(), &QQuickWindow::screenChanged, this, &DashboardWindow::visualParentScreenChanged);
    }

    m_visualParentWindow = window;

    if (m_visualParentWindow) {
        visualParentScreenChanged(m_visualParentWindow->screen());

        connect(m_visualParentWindow.data(), &QQuickWindow::screenChanged, this, &DashboardWindow::visualParentScreenChanged);
    }
}

void DashboardWindow::visualParentScreenChanged(QScreen *screen)
{
    if (screen) {
        setScreen(screen);
        setGeometry(screen->geometry());
    }
}
