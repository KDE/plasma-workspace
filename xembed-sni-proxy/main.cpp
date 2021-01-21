/*
 * Main
 * Copyright (C) 2015 <davidedmundson@kde.org> David Edmundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <QGuiApplication>
#include <QSessionManager>

#include "fdoselectionmanager.h"

#include "debug.h"
#include "snidbus.h"
#include "xcbutils.h"

#include <QDBusMetaType>

#include <KWindowSystem>

namespace Xcb
{
Xcb::Atoms *atoms;
}

int main(int argc, char **argv)
{
    // the whole point of this is to interact with X, if we are in any other session, force trying to connect to X
    // if the QPA can't load xcb, this app is useless anyway.
    qputenv("QT_QPA_PLATFORM", "xcb");

    QGuiApplication::setDesktopSettingsAware(false);

    QGuiApplication app(argc, argv);

    if (!KWindowSystem::isPlatformX11()) {
        qFatal("xembed-sni-proxy is only useful XCB. Aborting");
    }

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    app.setQuitOnLastWindowClosed(false);

    qDBusRegisterMetaType<KDbusImageStruct>();
    qDBusRegisterMetaType<KDbusImageVector>();
    qDBusRegisterMetaType<KDbusToolTipStruct>();

    Xcb::atoms = new Xcb::Atoms();

    FdoSelectionManager manager;

    auto rc = app.exec();

    delete Xcb::atoms;
    return rc;
}
