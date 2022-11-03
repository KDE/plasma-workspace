/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kickerplugin.h"
#include "abstractmodel.h"
#include "appsmodel.h"
#include "computermodel.h"
#include "containmentinterface.h"
#include "dashboardwindow.h"
#include "draghelper.h"
#include "funnelmodel.h"
#include "kastatsfavoritesmodel.h"
#include "kickercompattrianglemousefilter.h"
#include "processrunner.h"
#include "recentusagemodel.h"
#include "rootmodel.h"
#include "runnermodel.h"
#include "simplefavoritesmodel.h"
#include "submenu.h"
#include "systemmodel.h"
#include "systemsettings.h"
#include "wheelinterceptor.h"
#include "windowsystem.h"

void KickerPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QLatin1String(uri) == QLatin1String("org.kde.plasma.private.kicker"));

    qmlRegisterAnonymousType<AbstractModel>("", 1);

    qmlRegisterType<AppsModel>(uri, 0, 1, "AppsModel");
    qmlRegisterType<ComputerModel>(uri, 0, 1, "ComputerModel");
    qmlRegisterType<ContainmentInterface>(uri, 0, 1, "ContainmentInterface");
    qmlRegisterType<DragHelper>(uri, 0, 1, "DragHelper");
    qmlRegisterType<SimpleFavoritesModel>(uri, 0, 1, "FavoritesModel");
    qmlRegisterType<KAStatsFavoritesModel>(uri, 0, 1, "KAStatsFavoritesModel");
    qmlRegisterType<DashboardWindow>(uri, 0, 1, "DashboardWindow");
    qmlRegisterType<FunnelModel>(uri, 0, 1, "FunnelModel");
    qmlRegisterType<ProcessRunner>(uri, 0, 1, "ProcessRunner");
    qmlRegisterType<RecentUsageModel>(uri, 0, 1, "RecentUsageModel");
    qmlRegisterType<RootModel>(uri, 0, 1, "RootModel");
    qmlRegisterType<RunnerModel>(uri, 0, 1, "RunnerModel");
    qmlRegisterType<SubMenu>(uri, 0, 1, "SubMenu");
    qmlRegisterType<SystemModel>(uri, 0, 1, "SystemModel");
    qmlRegisterType<SystemSettings>(uri, 0, 1, "SystemSettings");
    qmlRegisterType<WheelInterceptor>(uri, 0, 1, "WheelInterceptor");
    qmlRegisterType<WindowSystem>(uri, 0, 1, "WindowSystem");
    qmlRegisterType<KickerCompatTriangleMouseFilter>(uri, 0, 1, "TriangleMouseFilter");
}
