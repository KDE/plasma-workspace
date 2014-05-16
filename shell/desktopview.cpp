/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "desktopview.h"
#include "containmentconfigview.h"
#include "shellcorona.h"
#include "shellmanager.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QScreen>

#include <kwindowsystem.h>
#include <klocalizedstring.h>

#include <Plasma/Package>

DesktopView::DesktopView(ShellCorona *corona, QScreen *screen)
    : PlasmaQuick::View(corona, 0),
      m_stayBehind(false),
      m_fillScreen(false),
      m_dashboardShown(false)
{
    setTitle(i18n("Desktop"));
    setIcon(QIcon::fromTheme("user-desktop"));
    setScreen(screen);
    engine()->rootContext()->setContextProperty("desktop", this);
    setSource(QUrl::fromLocalFile(corona->package().filePath("views", "Desktop.qml")));

    //For some reason, if I connect the method directly it doesn't get called, I think it's for the lack of argument
    connect(this, &QWindow::screenChanged, this, [=](QScreen*) { adaptToScreen(); ensureStayBehind(); });
}

DesktopView::~DesktopView()
{
}

bool DesktopView::stayBehind() const
{
    return m_stayBehind;
}

void DesktopView::setStayBehind(bool stayBehind)
{
    if (ShellManager::s_forceWindowed || stayBehind == m_stayBehind) {
        return;
    }

    m_stayBehind = stayBehind;
    ensureStayBehind();

    emit stayBehindChanged();
}

bool DesktopView::fillScreen() const
{
    return m_fillScreen;
}

void DesktopView::setFillScreen(bool fillScreen)
{
    if (ShellManager::s_forceWindowed || fillScreen == m_fillScreen) {
        return;
    }

    m_fillScreen = fillScreen;
    adaptToScreen();
    emit fillScreenChanged();
}

void DesktopView::moveEvent(QMoveEvent* ev)
{
    adaptToScreen();
    QWindow::moveEvent(ev);
}

void DesktopView::adaptToScreen()
{
    //This happens sometimes, when shutting down the process
    if (!screen())
        return;
//     qDebug() << "adapting to screen" << screen()->name() << this;
    if (m_fillScreen) {
        setGeometry(screen()->geometry());
        setMinimumSize(screen()->geometry().size());
        setMaximumSize(screen()->geometry().size());
        connect(screen(), &QScreen::geometryChanged, this, static_cast<void (QWindow::*)(const QRect&)>(&QWindow::setGeometry), Qt::UniqueConnection);
    } else {
        disconnect(screen(), &QScreen::geometryChanged, this, static_cast<void (QWindow::*)(const QRect&)>(&QWindow::setGeometry));
    }
//     qDebug() << "adapted" << geometry() << (containment () ? containment()->wallpaper() : "xx");
}

void DesktopView::ensureStayBehind()
{
    //This happens sometimes, when shutting down the process
    if (!screen())
        return;
    if (m_stayBehind) {
        KWindowSystem::setType(winId(), NET::Desktop);
    } else {
        KWindowSystem::setType(winId(), NET::Normal);
    }
}

void DesktopView::setDashboardShown(bool shown)
{
    if (shown) {
        if (m_stayBehind) {
            KWindowSystem::setType(winId(), NET::Normal);
            KWindowSystem::clearState(winId(), NET::SkipTaskbar|NET::KeepBelow);
        }
        setFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint);

        raise();
        KWindowSystem::raiseWindow(winId());
        KWindowSystem::forceActiveWindow(winId());

        QObject *wpGraphicObject = containment()->property("wallpaperGraphicsObject").value<QObject *>();
        if (wpGraphicObject) {
            wpGraphicObject->setProperty("opacity", 0.3);
        }
    } else {
        if (m_stayBehind) {
            KWindowSystem::setType(winId(), NET::Desktop);
            KWindowSystem::setState(winId(), NET::SkipTaskbar|NET::SkipPager|NET::KeepBelow);
        }
        lower();
        KWindowSystem::lowerWindow(winId());

        QObject *wpGraphicObject = containment()->property("wallpaperGraphicsObject").value<QObject *>();
        if (wpGraphicObject) {
            wpGraphicObject->setProperty("opacity", 1);
        }
    }

    m_dashboardShown = shown;
}

bool DesktopView::event(QEvent *e)
{
    if (e->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (m_dashboardShown && ke->key() == Qt::Key_Escape) {
            setDashboardShown(false);
        }
    } else if (e->type() == QEvent::Close) {
        //prevent ALT+F4 from killing the shell
        e->ignore();
        return true;
    } else if (e->type() == QEvent::FocusOut) {
        QObject *graphicObject = containment()->property("_plasma_graphicObject").value<QObject *>();
        if (graphicObject) {
            graphicObject->setProperty("focus", false);
        }
    }

    return PlasmaQuick::View::event(e);
}

bool DesktopView::isDashboardShown() const
{
    return m_dashboardShown;
}


void DesktopView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_configView) {
        m_configView.data()->hide();
        m_configView.data()->deleteLater();
    }

    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);

    if (cont && cont->isContainment()) {
        m_configView = new ContainmentConfigView(cont);
    } else {
        m_configView = new PlasmaQuick::ConfigView(applet);
    }
    m_configView.data()->init();
    m_configView.data()->show();
}

#include "moc_desktopview.cpp"
