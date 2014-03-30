/*
 *  Copyright 2014 Marco Martin <mart@kde.org>
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

#include "view.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QScreen>
#include <QQmlEngine>
#include <QClipboard>

#include <KWindowSystem>
#include <KWindowEffects>
#include <KAuthorized>
#include <KGlobalAccel>
#include <QAction>
#include <KLocalizedString>

#include <kdeclarative/qmlobject.h>

#include <Plasma/Package>

#include "appadaptor.h"

#include "shellpluginloader.h"

View::View(QWindow *parent)
    : PlasmaQuick::Dialog(),
      m_shownOnScreen(-1),
      m_offset(.5),
      m_floating(false)
{
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint);

    new AppAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/App"), this);

    if (KAuthorized::authorize(QLatin1String("run_command"))) {
        QAction *a = new QAction(0);
        QObject::connect(a, SIGNAL(triggered(bool)), SLOT(displayOrHide()));
        a->setText(i18n("Run Command"));
        a->setObjectName("run command");
        KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::Key_F2));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::Key_F2));

        a = new QAction(0);
        QObject::connect(a, SIGNAL(triggered(bool)), SLOT(displayWithClipboardContents()));
        a->setText(i18n("Run Command on clipboard contents"));
        a->setObjectName("run command on clipboard contents");
        KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::SHIFT+Qt::Key_F2));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::SHIFT+Qt::Key_F2));
    }

    Plasma::PluginLoader::setPluginLoader(new ShellPluginLoader);
    Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
    pkg.setPath("org.kde.lookandfeel");

    m_qmlObj = new KDeclarative::QmlObject(this);
    m_qmlObj->setInitializationDelayed(true);
    m_qmlObj->setSource(QUrl::fromLocalFile(pkg.filePath("runcommandmainscript")));
    setMainItem(qobject_cast<QQuickItem *>(m_qmlObj->rootObject()));
    m_qmlObj->engine()->rootContext()->setContextProperty("runnerWindow", this);
    m_qmlObj->completeInitialization();

    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(screenGeometryChanged(int)));
    connect(QApplication::desktop(), SIGNAL(screenCountChanged(int)), this, SLOT(screenGeometryChanged(int)));

    connect(KWindowSystem::self(), SIGNAL(workAreaChanged()), this, SLOT(resetScreenPos()));

    connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(resetScreenPos()));

    if (m_qmlObj->rootObject()) {
        connect(m_qmlObj->rootObject(), SIGNAL(widthChanged()), this, SLOT(resetScreenPos()));
    }

    if (m_floating) {
        setLocation(Plasma::Types::Floating);
    } else {
        setLocation(Plasma::Types::TopEdge);
    }
}

View::~View()
{
}

void View::showEvent(QShowEvent *event)
{
    KWindowSystem::setOnAllDesktops(winId(), true);
    Dialog::showEvent(event);
    positionOnScreen();
}

void View::screenResized(int screen)
{
    if (isVisible() && screen == m_shownOnScreen) {
        positionOnScreen();
    }
}

void View::screenGeometryChanged(int screenCount)
{
    Q_UNUSED(screenCount)

    if (isVisible()) {
        positionOnScreen();
    }
}

void View::resetScreenPos()
{
    if (isVisible() && !m_floating) {
        positionOnScreen();
    }
}

void View::positionOnScreen()
{
    if (QApplication::desktop()->screenCount() < 2) {
        m_shownOnScreen = QApplication::desktop()->primaryScreen();
    } else if (isVisible()) {
        m_shownOnScreen = QApplication::desktop()->screenNumber(geometry().center());
    } else {
        m_shownOnScreen = QApplication::desktop()->screenNumber(QCursor::pos());
    }

    const QRect r = QApplication::desktop()->screenGeometry(m_shownOnScreen);

    if (m_floating && !m_customPos.isNull()) {
        int x = qBound(r.left(), m_customPos.x(), r.right() - width());
        int y = qBound(r.top(), m_customPos.y(), r.bottom() - height());
        setPosition(x, y);
        show();
        return;
    }

    const int w = width();
    int x = r.left() + (r.width() * m_offset) - (w / 2);

    int y = r.top();
    if (m_floating) {
        y += r.height() / 3;
    }

    x = qBound(r.left(), x, r.right() - width());
    y = qBound(r.top(), y, r.bottom() - height());

    setPosition(x, y);

    if (m_floating) {
        KWindowSystem::setOnDesktop(winId(), KWindowSystem::currentDesktop());
        //Turn the sliding effect off
        KWindowEffects::slideWindow(winId(), KWindowEffects::NoEdge, 0);
    } else {
        KWindowSystem::setOnAllDesktops(winId(), true);
        KWindowEffects::slideWindow(winId(), KWindowEffects::TopEdge, 0);
    }

    KWindowSystem::forceActiveWindow(winId());
    //qDebug() << "moving to" << m_screenPos[screen];
}

void View::displayOrHide()
{
    setVisible(!isVisible());
}

void View::display()
{
    setVisible(true);
}

void View::displaySingleRunner(const QString &runnerName)
{
    setVisible(true);

    m_qmlObj->rootObject()->setProperty("runner", runnerName);
    m_qmlObj->rootObject()->setProperty("query", QString());
}

void View::displayWithClipboardContents()
{
    setVisible(true);

    m_qmlObj->rootObject()->setProperty("runner", QString());
    m_qmlObj->rootObject()->setProperty("query", QApplication::clipboard()->text(QClipboard::Selection));
}

void View::query(const QString &term)
{
    setVisible(true);

    m_qmlObj->rootObject()->setProperty("runner", QString());
    m_qmlObj->rootObject()->setProperty("query", term);
}

void View::querySingleRunner(const QString &runnerName, const QString &term)
{
    setVisible(true);

    m_qmlObj->rootObject()->setProperty("runner", runnerName);
    m_qmlObj->rootObject()->setProperty("query", term);
}

void View::switchUser()
{
    //vHanda will fix this or he will pay beers at Akademy to at least all the plasma team
    //And will invite afiestas for dinner in the japanese place.
    return;
}

void View::initializeStartupNotification()
{
    //vHanda will pay me another dinner if he does not implement this one
    return;
}


#include "moc_view.cpp"
