/*
 *   Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "switchuserdialog.h"
#include "ksmserver_debug.h"

#include <kdisplaymanager.h>

#include <QDebug>
#include <QGuiApplication>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
#include <QX11Info>
#include <QScreen>
#include <QStandardPaths>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowEffects>
#include <KWindowSystem>
#include <KUser>
#include <KDeclarative/KDeclarative>

#include <KWayland/Client/surface.h>
#include <KWayland/Client/plasmashell.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>

KSMSwitchUserDialog::KSMSwitchUserDialog(KDisplayManager *dm, KWayland::Client::PlasmaShell *plasmaShell, QWindow *parent)
    : QQuickView(parent)
    , m_displayManager(dm)
    , m_waylandPlasmaShell(plasmaShell)
{
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

    setResizeMode(QQuickView::SizeRootObjectToView);

    QPoint globalPosition(QCursor::pos());
    foreach (QScreen *s, QGuiApplication::screens()) {
        if (s->geometry().contains(globalPosition)) {
            setScreen(s);
            break;
        }
    }

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

    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(engine());
    kdeclarative.setupBindings();
}

void KSMSwitchUserDialog::init()
{
    rootContext()->setContextProperty(QStringLiteral("screenGeometry"), screen()->geometry());

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage("Plasma/LookAndFeel");
    KConfigGroup cg(KSharedConfig::openConfig("kdeglobals"), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    if (!packageName.isEmpty()) {
        package.setPath(packageName);
    }

    const QString fileName = package.filePath("userswitchermainscript");

    if (QFile::exists(fileName)) {
        setSource(QUrl::fromLocalFile(fileName));
    } else {
        qCWarning(KSMSERVER) << "Couldn't find a theme for the Switch User dialog" << fileName;
        return;
    }

    if (!errors().isEmpty()) {
        qCWarning(KSMSERVER) << errors();
    }

    connect(rootObject(), SIGNAL(dismissed()), this, SIGNAL(dismissed()));
    connect(rootObject(), SIGNAL(ungrab()), this, SLOT(ungrab()));

    connect(screen(), &QScreen::geometryChanged, this, [this] {
        setGeometry(screen()->geometry());
    });

    QQuickView::show();
    requestActivate();

    KWindowSystem::setState(winId(), NET::SkipTaskbar|NET::SkipPager);

    // in case you change this make sure to adjust ungrab() also
    setKeyboardGrabEnabled(true);
}

bool KSMSwitchUserDialog::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        switch (static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType()) {
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
    return QQuickView::event(e);
}

void KSMSwitchUserDialog::setupWaylandIntegration()
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

void KSMSwitchUserDialog::ungrab()
{
    // Allow the screenlocker to grab them immediately
    setKeyboardGrabEnabled(false);
    setMouseGrabEnabled(false);
}
