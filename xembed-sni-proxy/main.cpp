/*
    Main
    SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <QGuiApplication>
#include <QSessionManager>

#include "fdoselectionmanager.h"

#include "config-workspace.h"
#include "debug.h"
#include "snidbus.h"
#include "xcbutils.h"

#ifdef None
#ifndef FIXX11H_None
#define FIXX11H_None
inline constexpr XID XNone = None;
#undef None
inline constexpr XID None = XNone;
#endif
#undef None
#endif

#include <QDBusMetaType>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
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

    KAboutData about(QStringLiteral("xembedsniproxy"), QString(), QStringLiteral(WORKSPACE_VERSION_STRING));
    KAboutData::setApplicationData(about);

    KCrash::initialize();

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    app.connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    app.connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    app.setQuitOnLastWindowClosed(false);

    qDBusRegisterMetaType<KDbusImageStruct>();
    qDBusRegisterMetaType<KDbusImageVector>();
    qDBusRegisterMetaType<KDbusToolTipStruct>();

    Xcb::atoms = new Xcb::Atoms();

    KDBusService service(KDBusService::Unique);
    FdoSelectionManager manager;

    auto rc = app.exec();

    delete Xcb::atoms;
    return rc;
}
