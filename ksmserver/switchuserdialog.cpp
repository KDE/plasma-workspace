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

#include <kdisplaymanager.h>

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QEventLoop>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
#include <QX11Info>
#include <QScreen>
#include <QStandardItemModel>
#include <QStandardPaths>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowSystem>
#include <KUser>
#include <KDeclarative/KDeclarative>

#include <X11/Xutil.h>
#include <X11/Xatom.h>

KSMSwitchUserDialog::KSMSwitchUserDialog(KDisplayManager *dm, QWindow *parent)
    : QQuickView(parent)
    , m_displayManager(dm)
{
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

    QPoint globalPosition(QCursor::pos());
    foreach (QScreen *s, QGuiApplication::screens()) {
        if (s->geometry().contains(globalPosition)) {
            setScreen(s);
            break;
        }
    }

    // Qt doesn't set this on unmanaged windows
    //FIXME: or does it?
    XChangeProperty( QX11Info::display(), winId(),
        XInternAtom( QX11Info::display(), "WM_WINDOW_ROLE", False ), XA_STRING, 8, PropModeReplace,
        (unsigned char *)"logoutdialog", strlen( "logoutdialog" ));


    rootContext()->setContextProperty(QStringLiteral("screenGeometry"), screen()->geometry());

    setModality(Qt::ApplicationModal);

    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(engine());
    //kdeclarative.initialize();
    kdeclarative.setupBindings();

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
        qWarning() << "Couldn't find a theme for the Switch User dialog" << fileName;
        return;
    }

    setPosition(screen()->virtualGeometry().center().x() - width() / 2,
                screen()->virtualGeometry().center().y() - height() / 2);

    if (!errors().isEmpty()) {
        qWarning() << errors();
    }

    connect(rootObject(), SIGNAL(dismissed()), this, SIGNAL(dismissed()));

    show();
    requestActivate();

    KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
}

void KSMSwitchUserDialog::exec()
{
    QEventLoop loop;
    connect(this, &KSMSwitchUserDialog::dismissed, &loop, &QEventLoop::quit);
    loop.exec();
}
