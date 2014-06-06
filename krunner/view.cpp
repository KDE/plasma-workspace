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
      m_offset(.5),
      m_floating(false)
{
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
        KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT + Qt::Key_Space), KGlobalAccel::NoAutoloading);
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT + Qt::Key_Space) << QKeySequence(Qt::ALT + Qt::Key_F2));

        a = new QAction(0);
        QObject::connect(a, SIGNAL(triggered(bool)), SLOT(displayWithClipboardContents()));
        a->setText(i18n("Run Command on clipboard contents"));
        a->setObjectName("run command on clipboard contents");
        KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::SHIFT+Qt::Key_F2));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::SHIFT+Qt::Key_F2));
    }

    ShellPluginLoader::init();
    Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
    pkg.setPath("org.kde.lookandfeel");

    m_qmlObj = new KDeclarative::QmlObject(this);
    m_qmlObj->setInitializationDelayed(true);
    m_qmlObj->setSource(QUrl::fromLocalFile(pkg.filePath("runcommandmainscript")));
    m_qmlObj->engine()->rootContext()->setContextProperty("runnerWindow", this);
    m_qmlObj->completeInitialization();
    setMainItem(qobject_cast<QQuickItem *>(m_qmlObj->rootObject()));

    auto controlScreen = [=](QScreen* screen) {
        connect(screen, SIGNAL(geometryChanged(QRect)), SLOT(screenGeometryChanged()));
        connect(screen, SIGNAL(destroyed(QObject*)), SLOT(screenGeometryChanged()));
        screenGeometryChanged();
    };
    foreach(QScreen* s, QGuiApplication::screens())
        controlScreen(s);
    connect(qApp, &QGuiApplication::screenAdded, this, controlScreen);

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

bool View::event(QEvent *event)
{
    // QXcbWindow overwrites the state in its show event. There are plans
    // to fix this in 5.4, but till then we must explicitly overwrite it
    // each time.
    const bool retval = Dialog::event(event);
    if (event->type() != QEvent::DeferredDelete) {
        KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager | NET::KeepAbove | NET::StaysOnTop);
    }
    return retval;
}

void View::showEvent(QShowEvent *event)
{
    KWindowSystem::setOnAllDesktops(winId(), true);
    KWindowSystem::setState(winId(), NET::KeepAbove);
    Dialog::showEvent(event);
    positionOnScreen();
    requestActivate();
}

void View::screenGeometryChanged()
{
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
    QScreen* shownOnScreen = 0;
    if (QGuiApplication::screens().count() <= 1) {
        shownOnScreen = QGuiApplication::screens().first();
    } else {
        Q_FOREACH (QScreen* screen, QGuiApplication::screens()) {
            if (screen->geometry().contains(QCursor::pos()))
                shownOnScreen = screen;
        }
    }
    Q_ASSERT(shownOnScreen);

    setScreen(shownOnScreen);
    const QRect r = shownOnScreen->availableGeometry();

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
        KWindowSystem::setType(winId(), NET::Normal);
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
    m_qmlObj->rootObject()->setProperty("query", QGuiApplication::clipboard()->text(QClipboard::Selection));
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
    setVisible(true);

    m_qmlObj->rootObject()->setProperty("runner", "desktopsessions");
    m_qmlObj->rootObject()->setProperty("query", "SESSIONS");
}

void View::initializeStartupNotification()
{
    //vHanda will pay me another dinner if he does not implement this one
    return;
}


#include "moc_view.cpp"
