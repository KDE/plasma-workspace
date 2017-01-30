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

#include <QAction>
#include <QGuiApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QScreen>
#include <QQmlEngine>
#include <QClipboard>
#include <QPlatformSurfaceEvent>

#include <KWindowSystem>
#include <KWindowEffects>
#include <KAuthorized>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KDirWatch>
#include <KCrash/KCrash>

#include <kdeclarative/qmlobject.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/plasmashell.h>

#include "appadaptor.h"

View::View(QWindow *)
    : PlasmaQuick::Dialog(),
      m_offset(.5),
      m_floating(false),
      m_plasmaShell(nullptr),
      m_plasmaShellSurface(nullptr)
{
    initWayland();
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    KCrash::setFlags(KCrash::AutoRestart);

    m_config = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("krunnerrc")), "General");

    setFreeFloating(m_config.readEntry("FreeFloating", false));
    reloadConfig();

    new AppAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/App"), this);

    QAction *a = new QAction(0);
    QObject::connect(a, &QAction::triggered, this, &View::displayOrHide);
    a->setText(i18n("Run Command"));
    a->setObjectName(QStringLiteral("run command"));
    a->setProperty("componentDisplayName", i18nc("Name for krunner shortcuts category", "Run Command"));
    KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT + Qt::Key_Space), KGlobalAccel::NoAutoloading);
    KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT + Qt::Key_Space) << QKeySequence(Qt::ALT + Qt::Key_F2) << Qt::Key_Search);

    a = new QAction(0);
    QObject::connect(a, &QAction::triggered, this, &View::displayWithClipboardContents);
    a->setText(i18n("Run Command on clipboard contents"));
    a->setObjectName(QStringLiteral("run command on clipboard contents"));
    a->setProperty("componentDisplayName", i18nc("Name for krunner shortcuts category", "Run Command"));
    KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::SHIFT+Qt::Key_F2));
    KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::SHIFT+Qt::Key_F2));

    m_qmlObj = new KDeclarative::QmlObject(this);
    m_qmlObj->setInitializationDelayed(true);
    connect(m_qmlObj, &KDeclarative::QmlObject::finished, this, &View::objectIncubated);

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    if (!packageName.isEmpty()) {
        package.setPath(packageName);
    }

    m_qmlObj->setSource(QUrl::fromLocalFile(package.filePath("runcommandmainscript")));
    m_qmlObj->engine()->rootContext()->setContextProperty(QStringLiteral("runnerWindow"), this);
    m_qmlObj->completeInitialization();

    auto screenRemoved = [this](QScreen* screen) {
        if (screen == this->screen()) {
            setScreen(qGuiApp->primaryScreen());
            hide();
        }
    };

    auto screenAdded = [this](QScreen* screen) {
        connect(screen, &QScreen::geometryChanged, this, &View::screenGeometryChanged);
        screenGeometryChanged();
    };

    foreach(QScreen* s, QGuiApplication::screens())
        screenAdded(s);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, screenAdded);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, screenRemoved);

    connect(KWindowSystem::self(), &KWindowSystem::workAreaChanged, this, &View::resetScreenPos);

    connect(this, &View::visibleChanged, this, &View::resetScreenPos);

    KDirWatch::self()->addFile(m_config.name());

    // Catch both, direct changes to the config file ...
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &View::reloadConfig);
    connect(KDirWatch::self(), &KDirWatch::created, this, &View::reloadConfig);

    if (m_floating) {
        setLocation(Plasma::Types::Floating);
    } else {
        setLocation(Plasma::Types::TopEdge);
    }

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &View::slotFocusWindowChanged);
}

View::~View()
{
}

void View::initWayland()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }
    using namespace KWayland::Client;
    auto connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        return;
    }
    Registry *registry = new Registry(this);
    registry->create(connection);
    QObject::connect(registry, &Registry::interfacesAnnounced, this,
        [registry, this] {
            const auto interface = registry->interface(Registry::Interface::PlasmaShell);
            if (interface.name != 0) {
                m_plasmaShell = registry->createPlasmaShell(interface.name, interface.version, this);
            }
        }
    );

    registry->setup();
    connection->roundtrip();
}

void View::objectIncubated()
{
    connect(m_qmlObj->rootObject(), SIGNAL(widthChanged()), this, SLOT(resetScreenPos()));
    setMainItem(qobject_cast<QQuickItem *>(m_qmlObj->rootObject()));
}

void View::slotFocusWindowChanged()
{
    if (!QGuiApplication::focusWindow()) {
        setVisible(false);
    }
}

bool View::freeFloating() const
{
    return m_floating;
}

void View::setFreeFloating(bool floating)
{
    if (m_floating == floating) {
        return;
    }

    m_floating = floating;
    if (m_floating) {
        setLocation(Plasma::Types::Floating);
    } else {
        setLocation(Plasma::Types::TopEdge);
    }

    positionOnScreen();
}

void View::reloadConfig()
{
    m_config.config()->reparseConfiguration();
    setFreeFloating(m_config.readEntry("FreeFloating", false));

    const QStringList history = m_config.readEntry("history", QStringList());
    if (m_history != history) {
        m_history = history;
        emit historyChanged();
    }
}

bool View::event(QEvent *event)
{
    // QXcbWindow overwrites the state in its show event. There are plans
    // to fix this in 5.4, but till then we must explicitly overwrite it
    // each time.
    const bool retval = Dialog::event(event);
    bool setState = event->type() == QEvent::Show;
    if (event->type() == QEvent::PlatformSurface) {
        if (auto e = dynamic_cast<QPlatformSurfaceEvent*>(event)) {
            setState = e->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated;
        }
    }
    if (setState) {
        KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
    }

    if (m_plasmaShell && event->type() == QEvent::PlatformSurface) {
        if (auto e = dynamic_cast<QPlatformSurfaceEvent*>(event)) {
            using namespace KWayland::Client;
            switch (e->surfaceEventType()) {
            case QPlatformSurfaceEvent::SurfaceCreated: {
                Surface *s = Surface::fromWindow(this);
                if (!s) {
                    return false;
                }
                m_plasmaShellSurface = m_plasmaShell->createSurface(s, this);
                break;
            }
            case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                delete m_plasmaShellSurface;
                m_plasmaShellSurface = nullptr;
                break;
            }
        }
    } else if (m_plasmaShellSurface && event->type() == QEvent::Move) {
        QMoveEvent *me = static_cast<QMoveEvent *>(event);
        m_plasmaShellSurface->setPosition(me->pos());
    }

    return retval;
}

void View::resizeEvent(QResizeEvent *event)
{
    if (event->oldSize().width() != event->size().width()) {
        positionOnScreen();
    }
}

void View::showEvent(QShowEvent *event)
{
    KWindowSystem::setOnAllDesktops(winId(), true);
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
    QScreen *shownOnScreen = QGuiApplication::primaryScreen();

    Q_FOREACH (QScreen* screen, QGuiApplication::screens()) {
        if (screen->geometry().contains(QCursor::pos(screen))) {
            shownOnScreen = screen;
            break;
        }
    }

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
    if (isVisible() && !QGuiApplication::focusWindow())  {
        KWindowSystem::forceActiveWindow(winId());
        return;
    }
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
    QDBusConnection::sessionBus().asyncCall(
        QDBusMessage::createMethodCall(QStringLiteral("org.kde.ksmserver"),
                                       QStringLiteral("/KSMServer"),
                                       QStringLiteral("org.kde.KSMServerInterface"),
                                       QStringLiteral("openSwitchUserDialog"))
    );
}

void View::displayConfiguration()
{
    QProcess::startDetached(QStringLiteral("kcmshell5"), QStringList() << QStringLiteral("plasmasearch"));
}

QStringList View::history() const
{
    return m_history;
}

void View::addToHistory(const QString &item)
{
    if (item == QLatin1String("SESSIONS")) {
        return;
    }

    if (!KAuthorized::authorize(QStringLiteral("lineedit_text_completion"))) {
        return;
    }

    m_history.removeOne(item);
    m_history.prepend(item);

    while (m_history.count() > 50) { // make configurable?
        m_history.removeLast();
    }

    emit historyChanged();
    writeHistory();
    m_config.sync();
}

void View::removeFromHistory(int index)
{
    if (index < 0 || index >= m_history.count()) {
        return;
    }

    m_history.removeAt(index);
    emit historyChanged();

    writeHistory();
}

void View::writeHistory()
{
    m_config.writeEntry("history", m_history);
}
