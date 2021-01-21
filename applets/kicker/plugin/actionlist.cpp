/***************************************************************************
 *   Copyright (C) 2013 by Aurélien Gâteau <agateau@kde.org>               *
 *   Copyright (C) 2014 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "actionlist.h"
#include "menuentryeditor.h"

#include <config-appstream.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KPropertiesDialog>
#include <KProtocolInfo>

#include <KActivities/Stats/Cleaning>
#include <KActivities/Stats/ResultSet>
#include <KActivities/Stats/Terms>

#include "containmentinterface.h"

#ifdef HAVE_APPSTREAMQT
#include <AppStreamQt/pool.h>
#endif

namespace KAStats = KActivities::Stats;

using namespace KAStats;
using namespace KAStats::Terms;

namespace Kicker
{
QVariantMap createActionItem(const QString &label, const QString &icon, const QString &actionId, const QVariant &argument)
{
    QVariantMap map;

    map[QStringLiteral("text")] = label;
    map[QStringLiteral("icon")] = icon;
    map[QStringLiteral("actionId")] = actionId;

    if (argument.isValid()) {
        map[QStringLiteral("actionArgument")] = argument;
    }

    return map;
}

QVariantMap createTitleActionItem(const QString &label)
{
    QVariantMap map;

    map[QStringLiteral("text")] = label;
    map[QStringLiteral("type")] = QStringLiteral("title");

    return map;
}

QVariantMap createSeparatorActionItem()
{
    QVariantMap map;

    map[QStringLiteral("type")] = QStringLiteral("separator");

    return map;
}

QVariantList createActionListForFileItem(const KFileItem &fileItem)
{
    QVariantList list;

    const KService::List services = KApplicationTrader::queryByMimeType(fileItem.mimetype());

    if (!services.isEmpty()) {
        list << createTitleActionItem(i18n("Open with:"));

        for (const KService::Ptr service : services) {
            const QString text = service->name().replace(QLatin1Char('&'), QStringLiteral("&&"));
            QVariantMap item = createActionItem(text, service->icon(), QStringLiteral("_kicker_fileItem_openWith"), service->entryPath());

            list << item;
        }

        list << createSeparatorActionItem();
    }

    const QVariantMap &propertiesItem =
        createActionItem(i18n("Properties"), QStringLiteral("document-properties"), QStringLiteral("_kicker_fileItem_properties"));
    list << propertiesItem;

    return list;
}

bool handleFileItemAction(const KFileItem &fileItem, const QString &actionId, const QVariant &argument, bool *close)
{
    if (actionId == QLatin1String("_kicker_fileItem_properties")) {
        KPropertiesDialog *dlg = new KPropertiesDialog(fileItem, QApplication::activeWindow());
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();

        *close = false;

        return true;
    }

    if (actionId == QLatin1String("_kicker_fileItem_openWith")) {
        const QString path = argument.toString();
        const KService::Ptr service = KService::serviceByDesktopPath(path);

        if (!service) {
            return false;
        }

        auto *job = new KIO::ApplicationLauncherJob(service);
        job->setUrls({fileItem.url()});
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->start();

        *close = true;

        return true;
    }

    return false;
}

QVariantList createAddLauncherActionList(QObject *appletInterface, const KService::Ptr &service)
{
    QVariantList actionList;
    if (!service) {
        return actionList;
    }

    if (ContainmentInterface::mayAddLauncher(appletInterface, ContainmentInterface::Desktop)) {
        QVariantMap addToDesktopAction = Kicker::createActionItem(i18n("Add to Desktop"), QStringLiteral("list-add"), QStringLiteral("addToDesktop"));
        actionList << addToDesktopAction;
    }

    if (ContainmentInterface::mayAddLauncher(appletInterface, ContainmentInterface::Panel)) {
        QVariantMap addToPanelAction = Kicker::createActionItem(i18n("Add to Panel (Widget)"), QStringLiteral("list-add"), QStringLiteral("addToPanel"));
        actionList << addToPanelAction;
    }

    if (service && ContainmentInterface::mayAddLauncher(appletInterface, ContainmentInterface::TaskManager, Kicker::resolvedServiceEntryPath(service))) {
        QVariantMap addToTaskManagerAction = Kicker::createActionItem(i18n("Pin to Task Manager"), QStringLiteral("pin"), QStringLiteral("addToTaskManager"));
        actionList << addToTaskManagerAction;
    }

    return actionList;
}

bool handleAddLauncherAction(const QString &actionId, QObject *appletInterface, const KService::Ptr &service)
{
    if (!service) {
        return false;
    }

    if (actionId == QLatin1String("addToDesktop")) {
        if (ContainmentInterface::mayAddLauncher(appletInterface, ContainmentInterface::Desktop)) {
            ContainmentInterface::addLauncher(appletInterface, ContainmentInterface::Desktop, Kicker::resolvedServiceEntryPath(service));
        }
        return true;
    } else if (actionId == QLatin1String("addToPanel")) {
        if (ContainmentInterface::mayAddLauncher(appletInterface, ContainmentInterface::Panel)) {
            ContainmentInterface::addLauncher(appletInterface, ContainmentInterface::Panel, Kicker::resolvedServiceEntryPath(service));
        }
        return true;
    } else if (actionId == QLatin1String("addToTaskManager")) {
        if (ContainmentInterface::mayAddLauncher(appletInterface, ContainmentInterface::TaskManager, Kicker::resolvedServiceEntryPath(service))) {
            ContainmentInterface::addLauncher(appletInterface, ContainmentInterface::TaskManager, Kicker::resolvedServiceEntryPath(service));
        }
        return true;
    }

    return false;
}

QString storageIdFromService(KService::Ptr service)
{
    QString storageId = service->storageId();

    if (storageId.endsWith(QLatin1String(".desktop"))) {
        storageId = storageId.left(storageId.length() - 8);
    }

    return storageId;
}

QVariantList jumpListActions(KService::Ptr service)
{
    QVariantList list;

    if (!service) {
        return list;
    }

    // Add frequently used settings modules similar to SystemSetting's overview page.
    if (service->storageId() == QLatin1String("systemsettings.desktop")) {
        list = systemSettingsActions();

        if (!list.isEmpty()) {
            return list;
        }
    }

    const auto &actions = service->actions();
    for (const KServiceAction &action : actions) {
        if (action.text().isEmpty() || action.exec().isEmpty()) {
            continue;
        }

        QVariantMap item = createActionItem(action.text(), action.icon(), QStringLiteral("_kicker_jumpListAction"), action.exec());

        list << item;
    }

    return list;
}

QVariantList systemSettingsActions()
{
    QVariantList list;

    auto query = AllResources | Agent(QStringLiteral("org.kde.systemsettings")) | HighScoredFirst | Limit(5);

    ResultSet results(query);

    QStringList ids;
    for (const ResultSet::Result &result : results) {
        ids << QUrl(result.resource()).path();
    }

    if (ids.count() < 5) {
        // We'll load the default set of settings from its jump list actions.
        return list;
    }

    for (const QString &id : qAsConst(ids)) {
        KService::Ptr service = KService::serviceByStorageId(id);
        if (!service || !service->isValid()) {
            continue;
        }

        list << createActionItem(service->name(), service->icon(), QStringLiteral("_kicker_jumpListAction"), service->exec());
    }

    return list;
}

QVariantList recentDocumentActions(KService::Ptr service)
{
    QVariantList list;

    if (!service) {
        return list;
    }

    const QString storageId = storageIdFromService(service);

    if (storageId.isEmpty()) {
        return list;
    }

    // clang-format off
    auto query = UsedResources
        | RecentlyUsedFirst
        | Agent(storageId)
        | Type::any()
        | Activity::current()
        | Url::file();
    // clang-format on

    ResultSet results(query);

    ResultSet::const_iterator resultIt;
    resultIt = results.begin();

    while (list.count() < 6 && resultIt != results.end()) {
        const QString resource = (*resultIt).resource();
        ++resultIt;

        const QUrl url(resource);

        if (!url.isValid()) {
            continue;
        }

        const KFileItem fileItem(url);

        if (!fileItem.isFile()) {
            continue;
        }

        if (list.isEmpty()) {
            list << createTitleActionItem(i18n("Recent Files"));
        }

        QVariantMap item = createActionItem(url.fileName(), fileItem.iconName(), QStringLiteral("_kicker_recentDocument"), resource);

        list << item;
    }

    if (!list.isEmpty()) {
        QVariantMap forgetAction =
            createActionItem(i18n("Forget Recent Files"), QStringLiteral("edit-clear-history"), QStringLiteral("_kicker_forgetRecentDocuments"));
        list << forgetAction;
    }

    return list;
}

bool handleRecentDocumentAction(KService::Ptr service, const QString &actionId, const QVariant &_argument)
{
    if (!service) {
        return false;
    }

    if (actionId == QLatin1String("_kicker_forgetRecentDocuments")) {
        const QString storageId = storageIdFromService(service);

        if (storageId.isEmpty()) {
            return false;
        }

        // clang-format off
        auto query = UsedResources
            | Agent(storageId)
            | Type::any()
            | Activity::current()
            | Url::file();
        // clang-format on

        KAStats::forgetResources(query);

        return false;
    }

    QString argument = _argument.toString();

    if (argument.isEmpty()) {
        return false;
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    job->setUrls({QUrl(argument)});
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
    return job->exec();
}

Q_GLOBAL_STATIC(MenuEntryEditor, menuEntryEditor)

bool canEditApplication(const KService::Ptr &service)
{
    return (service->isApplication() && menuEntryEditor->canEdit(service->entryPath()));
}

void editApplication(const QString &entryPath, const QString &menuId)
{
    menuEntryEditor->edit(entryPath, menuId);
}

QVariantList editApplicationAction(const KService::Ptr &service)
{
    QVariantList actionList;

    if (canEditApplication(service)) {
        // TODO: Using the KMenuEdit icon might be misleading.
        QVariantMap editAction = Kicker::createActionItem(i18n("Edit Application..."), QStringLiteral("kmenuedit"), QStringLiteral("editApplication"));
        actionList << editAction;
    }

    return actionList;
}

bool handleEditApplicationAction(const QString &actionId, const KService::Ptr &service)
{
    if (service && actionId == QLatin1String("editApplication") && canEditApplication(service)) {
        Kicker::editApplication(service->entryPath(), service->menuId());

        return true;
    }

    return false;
}

#ifdef HAVE_APPSTREAMQT
Q_GLOBAL_STATIC(AppStream::Pool, appstreamPool)
#endif

QVariantList appstreamActions(const KService::Ptr &service)
{
    QVariantList ret;

#ifdef HAVE_APPSTREAMQT
    const KService::Ptr appStreamHandler = KApplicationTrader::preferredService(QStringLiteral("x-scheme-handler/appstream"));

    // Don't show action if we can't find any app to handle appstream:// URLs.
    if (!appStreamHandler) {
        if (!KProtocolInfo::isHelperProtocol(QStringLiteral("appstream")) || KProtocolInfo::exec(QStringLiteral("appstream")).isEmpty()) {
            return ret;
        }
    }

    if (!appstreamPool.exists()) {
        appstreamPool->load();
    }

    const auto components = appstreamPool->componentsById(service->desktopEntryName() + QLatin1String(".desktop"));
    for (const auto &component : components) {
        const QString componentId = component.id();

        QVariantMap appstreamAction = Kicker::createActionItem(i18nc("@action opens a software center with the application", "Uninstall or Manage Add-Ons..."),
                                                               appStreamHandler->icon(),
                                                               "manageApplication",
                                                               QVariant(QLatin1String("appstream://") + componentId));
        ret << appstreamAction;
    }
#else
    Q_UNUSED(service)
#endif

    return ret;
}

bool handleAppstreamActions(const QString &actionId, const QVariant &argument)
{
    if (actionId == QLatin1String("manageApplication")) {
        return QDesktopServices::openUrl(QUrl(argument.toString()));
    }

    return false;
}

QString resolvedServiceEntryPath(const KService::Ptr &service)
{
    QString path = service->entryPath();
    if (!QDir::isAbsolutePath(path)) {
        path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("kservices5/") + path);
    }
    return path;
}

}
