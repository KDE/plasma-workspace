/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "splashwindow.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <KSharedConfig>

#include <Plasma/Package>
#include <Plasma/PluginLoader>

SplashWindow::SplashWindow(bool testing, bool window)
    : QQuickView(),
      m_stage(0),
      m_testing(testing),
      m_window(window)
{
    setColor(Qt::transparent);
    setDefaultAlphaBuffer(true);
    setClearBeforeRendering(true);
    setResizeMode(QQuickView::SizeRootObjectToView);

    if (!m_window) {
        setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    }

    if (!m_testing && !m_window) {
        setFlags(Qt::BypassWindowManagerHint);
    }

    if (m_testing && !m_window) {
        setWindowState(Qt::WindowFullScreen);
    }

    //be sure it will be eventually closed
    //FIXME: should never be stuck
    QTimer::singleShot(30000, this, SLOT(close()));
}

void SplashWindow::setStage(int stage)
{
    m_stage = stage;

    rootObject()->setProperty("stage", stage);
}

void SplashWindow::keyPressEvent(QKeyEvent *event)
{
    QQuickView::keyPressEvent(event);
    if (m_testing && !event->isAccepted() && event->key() == Qt::Key_Escape) {
        close();
    }
}

void SplashWindow::mousePressEvent(QMouseEvent *event)
{
    QQuickView::mousePressEvent(event);
    if (m_testing && !event->isAccepted()) {
        close();
    }
}

void SplashWindow::setGeometry(const QRect& rect)
{
    bool oldGeometryEmpty = geometry().isNull();
    QQuickView::setGeometry(rect);

    if (oldGeometryEmpty) {

        Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
        KConfigGroup cg(KSharedConfig::openConfig("kdeglobals"), "KDE");
        const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
        if (!packageName.isEmpty()) {
            package.setPath(packageName);
        };

        const QString theme = QGuiApplication::arguments().at(1);
        if (!theme.startsWith("--")) {
            package.setPath(theme);
        }

        setSource(QUrl::fromLocalFile(package.filePath("splashmainscript")));
    }
}
