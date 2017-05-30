/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>
Copyright 2007 Urs Wolfer <uwolfer @ kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include "shutdowndlg.h"
//include <ksmserver_debug.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QQuickItem>
#include <QTimer>
#include <QFile>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QPainter>
#include <QStandardPaths>
#include <QtX11Extras/qx11info_x11.h>
#include <QScreen>
#include <QStandardPaths>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KAuthorized>
#include <KIconLoader>
#include <KLocalizedString>
#include <KUser>
#include <Solid/PowerManagement>
#include <KWindowEffects>
#include <KWindowSystem>
#include <KDeclarative/KDeclarative>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KJob>

#include <stdio.h>
#include <netwm.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <fixx11h.h>

#include <kdisplaymanager.h>

#include <config-workspace.h>

#include <KWayland/Client/surface.h>
#include <KWayland/Client/plasmashell.h>

Q_DECLARE_METATYPE(Solid::PowerManagement::SleepState)

KSMShutdownDlg::KSMShutdownDlg( QWindow* parent,
                                bool maysd, bool choose, KWorkSpace::ShutdownType sdtype,
                                const QString& theme, KWayland::Client::PlasmaShell *plasmaShell)
  : QQuickView(parent),
    m_result(false),
    m_theme(theme),
    m_waylandPlasmaShell(plasmaShell)
    // this is a WType_Popup on purpose. Do not change that! Not
    // having a popup here has severe side effects.
{
    // window stuff
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

    setResizeMode(QQuickView::SizeRootObjectToView);

    // Qt doesn't set this on unmanaged windows
    //FIXME: or does it?
    if (KWindowSystem::isPlatformX11()) {
        XChangeProperty( QX11Info::display(), winId(),
            XInternAtom( QX11Info::display(), "WM_WINDOW_ROLE", False ), XA_STRING, 8, PropModeReplace,
            (unsigned char *)"logoutdialog", strlen( "logoutdialog" ));

        XClassHint classHint;
        classHint.res_name = const_cast<char*>("ksmserver");
        classHint.res_class = const_cast<char*>("ksmserver");
        XSetClassHint(QX11Info::display(), winId(), &classHint);
    }

    //QQuickView *windowContainer = QQuickView::createWindowContainer(m_view, this);
    //windowContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QQmlContext *context = rootContext();
    context->setContextProperty(QStringLiteral("maysd"), maysd);
    context->setContextProperty(QStringLiteral("choose"), choose);
    context->setContextProperty(QStringLiteral("sdtype"), sdtype);

    QQmlPropertyMap *mapShutdownType = new QQmlPropertyMap(this);
    mapShutdownType->insert(QStringLiteral("ShutdownTypeDefault"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeDefault));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeNone"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeNone));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeReboot"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeReboot));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeHalt"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeHalt));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeLogout"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeLogout));
    context->setContextProperty(QStringLiteral("ShutdownType"), mapShutdownType);

    QQmlPropertyMap *mapSpdMethods = new QQmlPropertyMap(this);
    QSet<Solid::PowerManagement::SleepState> spdMethods = Solid::PowerManagement::supportedSleepStates();
    mapSpdMethods->insert(QStringLiteral("StandbyState"), QVariant::fromValue(spdMethods.contains(Solid::PowerManagement::StandbyState)));
    mapSpdMethods->insert(QStringLiteral("SuspendState"), QVariant::fromValue(spdMethods.contains(Solid::PowerManagement::SuspendState)));
    mapSpdMethods->insert(QStringLiteral("HibernateState"), QVariant::fromValue(spdMethods.contains(Solid::PowerManagement::HibernateState)));
    context->setContextProperty(QStringLiteral("spdMethods"), mapSpdMethods);
    context->setContextProperty(QStringLiteral("canLogout"), KAuthorized::authorize(QStringLiteral("logout")));

    QString bootManager = KConfig(QStringLiteral(KDE_CONFDIR "/kdm/kdmrc"), KConfig::SimpleConfig)
                          .group("Shutdown")
                          .readEntry("BootManager", "None");
    context->setContextProperty(QStringLiteral("bootManager"), bootManager);

    QStringList options;
    int def, cur;
    if ( KDisplayManager().bootOptions( rebootOptions, def, cur ) ) {
        if ( cur > -1 ) {
            def = cur;
        }
    }
    QQmlPropertyMap *rebootOptionsMap = new QQmlPropertyMap(this);
    rebootOptionsMap->insert(QStringLiteral("options"), QVariant::fromValue(rebootOptions));
    rebootOptionsMap->insert(QStringLiteral("default"), QVariant::fromValue(def));
    context->setContextProperty(QStringLiteral("rebootOptions"), rebootOptionsMap);

    // engine stuff
    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(engine());
    kdeclarative.initialize();
    kdeclarative.setupBindings();
//    windowContainer->installEventFilter(this);
}

void KSMShutdownDlg::init()
{
    rootContext()->setContextProperty(QStringLiteral("screenGeometry"), screen()->geometry());

    QString fileName;
    if(m_theme.isEmpty()) {
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
        KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
        const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
        if (!packageName.isEmpty()) {
            package.setPath(packageName);
        }

        fileName = package.filePath("logoutmainscript");
    } else
        fileName = m_theme;

    if (QFile::exists(fileName)) {
        //qCDebug(KSMSERVER) << "Using QML theme" << fileName;
        setSource(QUrl::fromLocalFile(fileName));
    } else {
        qWarning() << "Couldn't find a theme for the Shutdown dialog" << fileName;
        return;
    }

    if(!errors().isEmpty()) {
        qWarning() << errors();
    }

    connect(rootObject(), SIGNAL(logoutRequested()), SLOT(slotLogout()));
    connect(rootObject(), SIGNAL(haltRequested()), SLOT(slotHalt()));
    connect(rootObject(), SIGNAL(suspendRequested(int)), SLOT(slotSuspend(int)) );
    connect(rootObject(), SIGNAL(rebootRequested()), SLOT(slotReboot()));
    connect(rootObject(), SIGNAL(rebootRequested2(int)), SLOT(slotReboot(int)) );
    connect(rootObject(), SIGNAL(cancelRequested()), SLOT(reject()));
    connect(rootObject(), SIGNAL(lockScreenRequested()), SLOT(slotLockScreen()));

    connect(screen(), &QScreen::geometryChanged, this, [this] {
        setGeometry(screen()->geometry());
    });

    QQuickView::show();
    requestActivate();

    KWindowSystem::setState(winId(), NET::SkipTaskbar|NET::SkipPager);

    setKeyboardGrabEnabled(true);
}

void KSMShutdownDlg::resizeEvent(QResizeEvent *e)
{
    QQuickView::resizeEvent( e );

    if( KWindowSystem::compositingActive()) {
        //TODO: reenable window mask when we are without composite?
//        clearMask();
    } else {
//        setMask(m_view->mask());
    }
}

bool KSMShutdownDlg::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        if (auto pe = dynamic_cast<QPlatformSurfaceEvent*>(e)) {
            switch (pe->surfaceEventType()) {
            case QPlatformSurfaceEvent::SurfaceCreated:
                setupWaylandIntegration();
                KWindowEffects::enableBlurBehind(winId(), true);
                break;
            case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                delete m_shellSurface;
                m_shellSurface = nullptr;
                break;
            }
        }
    }
    return QQuickView::event(e);
}

void KSMShutdownDlg::setupWaylandIntegration()
{
    if (m_shellSurface) {
        // already setup
        return;
    }
    using namespace KWayland::Client;
    if (!m_waylandPlasmaShell) {
        return;
    }
    Surface *s = Surface::fromWindow(this);
    if (!s) {
        return;
    }
    m_shellSurface = m_waylandPlasmaShell->createSurface(s, this);
    // TODO: set a proper window type to indicate to KWin that this is the logout dialog
    // maybe we need a dedicated type for it?
    m_shellSurface->setPosition(geometry().topLeft());
}

void KSMShutdownDlg::slotLogout()
{
    m_shutdownType = KWorkSpace::ShutdownTypeNone;
    accept();
}

void KSMShutdownDlg::slotReboot()
{
    // no boot option selected -> current
    m_bootOption.clear();
    m_shutdownType = KWorkSpace::ShutdownTypeReboot;
    accept();
}

void KSMShutdownDlg::slotReboot(int opt)
{
    if (int(rebootOptions.size()) > opt)
        m_bootOption = rebootOptions[opt];
    m_shutdownType = KWorkSpace::ShutdownTypeReboot;
    accept();
}


void KSMShutdownDlg::slotLockScreen()
{
    m_bootOption.clear();
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.kde.screensaver"),
                                                       QStringLiteral("/ScreenSaver"),
                                                       QStringLiteral("org.freedesktop.ScreenSaver"),
                                                       QStringLiteral("Lock"));
    QDBusConnection::sessionBus().asyncCall(call);
    reject();
}

void KSMShutdownDlg::slotHalt()
{
    m_bootOption.clear();
    m_shutdownType = KWorkSpace::ShutdownTypeHalt;
    accept();
}

void KSMShutdownDlg::slotSuspend(int spdMethod)
{
    m_bootOption.clear();
    switch (spdMethod) {
        case Solid::PowerManagement::StandbyState:
        case Solid::PowerManagement::SuspendState:
            Solid::PowerManagement::requestSleep(Solid::PowerManagement::SuspendState, 0, 0);
            break;
        case Solid::PowerManagement::HibernateState:
            Solid::PowerManagement::requestSleep(Solid::PowerManagement::HibernateState, 0, 0);
            break;
    }
    reject();
}

void KSMShutdownDlg::accept()
{
    emit accepted();
}

void KSMShutdownDlg::reject()
{
    emit rejected();
}
