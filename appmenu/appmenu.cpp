/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "appmenu.h"
#include "kdbusimporter.h"
#include "menuimporteradaptor.h"
#include "appmenuadaptor.h"
#include "appmenu_dbus.h"
#if 0
#include "topmenubar.h"
#endif
#include "verticalmenu.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusPendingCallWatcher>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>

#include <KDebug>
#include <KWindowSystem>
#include <KWindowInfo>
#include <KConfig>
#include <KConfigGroup>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <netwm.h>

K_PLUGIN_FACTORY_WITH_JSON(AppMenuFactory,
                           "appmenu.json",
                           registerPlugin<AppMenuModule>();)

AppMenuModule::AppMenuModule(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent),
    m_parent(parent),
    m_menuImporter(0),
    m_appmenuDBus(new AppmenuDBus(parent)),
    m_menubar(0),
    m_menu(0),
    m_screenTimer(new QTimer(this)),
    m_waitingAction(0),
    m_currentScreen(-1)
{
    reconfigure();

    m_appmenuDBus->connectToBus();

    m_currentScreen = currentScreen();

    connect(m_appmenuDBus, &AppmenuDBus::appShowMenu, this, &AppMenuModule::slotShowMenu);
    connect(m_appmenuDBus, &AppmenuDBus::moduleReconfigure, this, &AppMenuModule::reconfigure);

    // transfer our signals to dbus
    connect(this, &AppMenuModule::showRequest, m_appmenuDBus, &AppmenuDBus::showRequest);
    connect(this, &AppMenuModule::menuAvailable, m_appmenuDBus, &AppmenuDBus::menuAvailable);
    connect(this, &AppMenuModule::clearMenus, m_appmenuDBus, &AppmenuDBus::clearMenus);
    connect(this, &AppMenuModule::menuHidden, m_appmenuDBus, &AppmenuDBus::menuHidden);
    connect(this, &AppMenuModule::WindowRegistered,
            m_appmenuDBus, &AppmenuDBus::WindowRegistered);
    connect(this, &AppMenuModule::WindowUnregistered, m_appmenuDBus, &AppmenuDBus::WindowUnregistered);
}

AppMenuModule::~AppMenuModule()
{
    emit clearMenus();
    hideMenubar();
#if 0
    delete m_menubar;
#endif
    delete m_menuImporter;
    delete m_appmenuDBus;
}

void AppMenuModule::slotShowMenu(int x, int y, WId id)
{
    static KDBusMenuImporter *importer = 0;

    if (!m_menuImporter) {
        return;
    }

    // If menu visible, hide it
    if (m_menu && m_menu->isVisible()) {
        m_menu->hide();
        return;
    }

    //dbus call by user (for khotkey shortcut)
    if (x == -1 || y == -1) {
        // We do not know kwin button position, so tell kwin to show menu
        emit showRequest(KWindowSystem::self()->activeWindow());
        return;
    }

    importer = getImporter(id);

    if (!importer) {
        return;
    }

    QMenu *menu = importer->menu();

    // Window do not have menu
    if (!menu) {
        return;
    }

    m_menu = new VerticalMenu();
    m_menu->setParentWid(id);
    // Populate menu
    foreach (QAction *action, menu->actions()) {
        m_menu->addAction(action);
    }
    m_menu->popup(QPoint(x, y));
    // Activate waiting action if exist
    if (m_waitingAction) {
        m_menu->setActiveAction(m_waitingAction);
        m_waitingAction = 0;
    }
    connect(m_menu, &QMenu::aboutToHide, this, &AppMenuModule::slotAboutToHide);
}

void AppMenuModule::slotAboutToHide()
{
    if (m_menu) {
        emit menuHidden(m_menu->parentWid());
        m_menu->deleteLater();
        m_menu = 0;
    }
}

// New window registered
void AppMenuModule::slotWindowRegistered(WId id, const QString& service, const QDBusObjectPath& path)
{
    KDBusMenuImporter* importer = m_importers.take(id);
     if (importer) {
        delete importer;
    }

    // Application already active so check if we need create menubar
    if ( m_menuStyle == QLatin1String("TopMenuBar") && id == KWindowSystem::self()->activeWindow()) {
        slotActiveWindowChanged(id);
    } else if (m_menuStyle == QLatin1String("ButtonVertical")) {
        KWindowInfo info(id, 0, NET::WM2WindowClass);
        // Tell Kwin menu is available
        emit menuAvailable(id);
        // FIXME: https://bugs.kde.org/show_bug.cgi?id=317926
        if (info.windowClassName() != "kmix") {
            getImporter(id);
        }
    }

    // Send a signal on bus for others dbus interface registrars
    emit WindowRegistered(id, service, path);
}

// Window unregistered
void AppMenuModule::slotWindowUnregistered(WId id)
{
    KDBusMenuImporter* importer = m_importers.take(id);

    // Send a signal on bus for others dbus interface registrars
    emit WindowUnregistered(id);

    if (importer) {
        delete importer;
    }

#if 0
    if (m_menubar && m_menubar->parentWid() == id) {
        hideMenubar();
    }
#endif
}

// Keyboard activation requested, transmit it to menu
void AppMenuModule::slotActionActivationRequested(QAction* a)
{
    // If we have a topmenubar, activate action
#if 0
    if (m_menubar) {
        m_menubar->setActiveAction(a);
        m_menubar->show();
    } else
#endif
    { // else send request to kwin or others dbus interface registrars
        m_waitingAction = a;
        emit showRequest(KWindowSystem::self()->activeWindow());
    }
}

// Current window change, update menubar
// See comments in slotWindowRegistered() for why we get importers here
void AppMenuModule::slotActiveWindowChanged(WId id)
{
    KWindowInfo info(id, NET::WMWindowType);
    NET::WindowTypes mask = NET::AllTypesMask;

    m_currentScreen = currentScreen();

    if (id == 0) {// Ignore root window
        return;
    } else if (info.windowType(mask) & NET::Dock) { // Hide immediatly menubar for docks (krunner)
        hideMenubar();
        return;
    }

    if (!m_menuImporter->serviceExist(id)) { // No menu exist, check for another menu for application
        WId recursiveId = m_menuImporter->recursiveMenuId(id);
        if (recursiveId) {
           id = recursiveId;
        }
    }

    KDBusMenuImporter *importer = getImporter(id);
    if (!importer) {
        hideMenubar();
        return;
    }

#if 0
    QMenu *menu = importer->menu();

    if(menu) {
        showMenuBar(menu);
        m_menubar->setParentWid(id);
    } else {
        hideMenubar();
    }
#endif
}

void AppMenuModule::slotShowCurrentWindowMenu()
{
    slotActiveWindowChanged(KWindowSystem::self()->activeWindow());
}

void AppMenuModule::slotCurrentScreenChanged()
{
    if (m_currentScreen != currentScreen()) {
#if 0
        if (m_menubar) {
            m_menubar->setParentWid(0);
        }
#endif
        slotActiveWindowChanged(KWindowSystem::self()->activeWindow());
    }
}

void AppMenuModule::slotBarNeedResize()
{
#if 0
    if (m_menubar) {
        m_menubar->updateSize();
        m_menubar->move(centeredMenubarPos());
    }
#endif
}

// reload settings
void AppMenuModule::reconfigure()
{
    KConfig config( QStringLiteral("kdeglobals"), KConfig::FullConfig );
    KConfigGroup configGroup = config.group("Appmenu Style");
    m_menuStyle = configGroup.readEntry("Style", "InApplication");

    m_waitingAction = 0;

    // hide menubar if exist
    hideMenubar();
#if 0
    delete m_menubar;
    m_menubar = 0;
#endif

    slotAboutToHide(); // hide vertical menu if exist

    // Disconnect all options specifics signals
    disconnect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this, &AppMenuModule::slotActiveWindowChanged);
    disconnect(KWindowSystem::self(), &KWindowSystem::workAreaChanged, this, &AppMenuModule::slotShowCurrentWindowMenu);
    disconnect(m_screenTimer, &QTimer::timeout, this, &AppMenuModule::slotCurrentScreenChanged);

    m_screenTimer->stop();

    // Tell kwin to clean its titlebar
    emit clearMenus();

    if (m_menuStyle == QLatin1String("InApplication")) {
        if (m_menuImporter) {
            delete m_menuImporter;
            m_menuImporter = 0;
        }
        return;
    }

    // Setup a menu importer if needed
    if (!m_menuImporter) {
        m_menuImporter = new MenuImporter(m_parent);
        connect(m_menuImporter, &MenuImporter::WindowRegistered,
            this, &AppMenuModule::slotWindowRegistered);
        connect(m_menuImporter, &MenuImporter::WindowUnregistered,
            this, &AppMenuModule::slotWindowUnregistered);
        m_menuImporter->connectToBus();
    }

    if( m_menuStyle == QLatin1String("ButtonVertical") ) {
        foreach(WId id, m_menuImporter->ids()) {
            emit menuAvailable(id);
        }
    }

    // Setup top menubar if needed
    if (m_menuStyle == QLatin1String("TopMenuBar")) {
#if 0
        m_menubar = new TopMenuBar();
        connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(slotActiveWindowChanged(WId)));
        connect(KWindowSystem::self(), SIGNAL(workAreaChanged()), this, SLOT(slotShowCurrentWindowMenu()));
        connect(m_screenTimer, SIGNAL(timeout()), this, SLOT(slotCurrentScreenChanged()));
        connect(m_menubar, SIGNAL(needResize()), SLOT(slotBarNeedResize()));
        m_screenTimer->start(1000);
        slotShowCurrentWindowMenu();
#endif
    }
}

KDBusMenuImporter* AppMenuModule::getImporter(WId id)
{
    KDBusMenuImporter* importer = 0;
    if (m_importers.contains(id)) { // importer already exist
        importer = m_importers.value(id);
    } else if (m_menuImporter->serviceExist(id)) { // get importer
        importer = new KDBusMenuImporter(id, m_menuImporter->serviceForWindow(id), &m_icons,
                                             m_menuImporter->pathForWindow(id), this);
        if (importer) {
            QMetaObject::invokeMethod(importer, "updateMenu", Qt::DirectConnection);
            connect(importer, &DBusMenuImporter::actionActivationRequested,
                    this, &AppMenuModule::slotActionActivationRequested);
            m_importers.insert(id, importer);
        }
    }
    return importer;
}

void AppMenuModule::showMenuBar(QMenu *menu)
{
#if 0
    if (!menu) {
        return;
    }

    m_menubar->setMenu(menu);
    if (menu->actions().length()) {
        m_menubar->enableMouseTracking();
    }
#endif
}

void AppMenuModule::hideMenubar()
{
#if 0
    if (!m_menubar) {
        return;
    }

    m_menubar->enableMouseTracking(false);
    if (m_menubar->isVisible()) {
        m_menubar->hide();
    }
#endif
}

int AppMenuModule::currentScreen()
{
    KWindowInfo info(KWindowSystem::self()->activeWindow(), NET::WMGeometry);
    int x = info.geometry().x();
    int y = info.geometry().y();

    QDesktopWidget *desktop = QApplication::desktop();
    return desktop->screenNumber(QPoint(x,y));
}


QPoint AppMenuModule::centeredMenubarPos()
{
    QDesktopWidget *desktop = QApplication::desktop();
    m_currentScreen = currentScreen();
    QRect screen = desktop->availableGeometry(m_currentScreen);
#if 0
    int x = screen.center().x() - m_menubar->sizeHint().width()/2;
    return QPoint(x, screen.topLeft().y());
#else
    return QPoint(screen.center().x(), screen.topLeft().y());
#endif
}


#include "appmenu.moc"
