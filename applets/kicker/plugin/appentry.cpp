/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "appentry.h"
#include "actionlist.h"
#include "appsmodel.h"
#include "containmentinterface.h"
#include <config-workspace.h>

#include <config-X11.h>

#include <QFileInfo>
#include <QProcess>
#include <QQmlPropertyMap>
#include <QStandardPaths>
#if HAVE_X11
#include <QX11Info>
#endif

#include <KActivities/ResourceInstance>
#include <KApplicationTrader>
#include <KConfigGroup>
#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>
#include <KJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KSharedConfig>
#include <KShell>
#include <KStartupInfo>
#include <KSycoca>
#include <KWindowSystem>

#include <Plasma/Plasma>

AppEntry::AppEntry(AbstractModel *owner, KService::Ptr service, NameFormat nameFormat)
    : AbstractEntry(owner)
    , m_service(service)
{
    if (m_service) {
        init(nameFormat);
    }
}

AppEntry::AppEntry(AbstractModel *owner, const QString &id)
    : AbstractEntry(owner)
{
    const QUrl url(id);

    if (url.scheme() == QLatin1String("preferred")) {
        m_service = defaultAppByName(url.host());
        m_id = id;
        m_con = QObject::connect(KSycoca::self(), &KSycoca::databaseChanged, owner, [this, owner, id]() {
            KSharedConfig::openConfig()->reparseConfiguration();
            m_service = defaultAppByName(QUrl(id).host());
            if (m_service) {
                init((NameFormat)owner->rootModel()->property("appNameFormat").toInt());
                m_icon = QIcon();
                Q_EMIT owner->layoutChanged();
            }
        });
    } else {
        m_service = KService::serviceByStorageId(id);
    }

    if (m_service) {
        init((NameFormat)owner->rootModel()->property("appNameFormat").toInt());
    }
}

void AppEntry::init(NameFormat nameFormat)
{
    m_name = nameFromService(m_service, nameFormat);

    if (nameFormat == GenericNameOnly) {
        m_description = nameFromService(m_service, NameOnly);
    } else {
        m_description = nameFromService(m_service, GenericNameOnly);
    }
}

bool AppEntry::isValid() const
{
    return m_service;
}

QIcon AppEntry::icon() const
{
    if (m_icon.isNull()) {
        if (QFileInfo::exists(m_service->icon())) {
            m_icon = QIcon(m_service->icon());
        } else {
            m_icon = QIcon::fromTheme(m_service->icon(), QIcon::fromTheme(QStringLiteral("unknown")));
        }
    }
    return m_icon;
}

QString AppEntry::name() const
{
    return m_name;
}

QString AppEntry::description() const
{
    return m_description;
}

KService::Ptr AppEntry::service() const
{
    return m_service;
}

QString AppEntry::id() const
{
    if (!m_id.isEmpty()) {
        return m_id;
    }

    return m_service->storageId();
}

QString AppEntry::menuId() const
{
    return m_service->menuId();
}

QUrl AppEntry::url() const
{
    return QUrl::fromLocalFile(Kicker::resolvedServiceEntryPath(m_service));
}

bool AppEntry::hasActions() const
{
    return true;
}

QVariantList AppEntry::actions() const
{
    QVariantList actionList;

    actionList << Kicker::jumpListActions(m_service);
    if (!actionList.isEmpty()) {
        actionList << Kicker::createSeparatorActionItem();
    }

    QObject *appletInterface = m_owner->rootModel()->property("appletInterface").value<QObject *>();

    bool systemImmutable = false;
    if (appletInterface) {
        systemImmutable = (appletInterface->property("immutability").toInt() == Plasma::Types::SystemImmutable);
    }

    const QVariantList &addLauncherActions = Kicker::createAddLauncherActionList(appletInterface, m_service);
    if (!systemImmutable && !addLauncherActions.isEmpty()) {
        actionList << addLauncherActions;
    }

    const QVariantList &recentDocuments = Kicker::recentDocumentActions(m_service);
    if (!recentDocuments.isEmpty()) {
        actionList << recentDocuments << Kicker::createSeparatorActionItem();
    }

    // Don't allow adding launchers, editing, hiding, or uninstalling applications
    // when system is immutable.
    if (systemImmutable) {
        return actionList;
    }

    if (m_service->isApplication()) {
        actionList << Kicker::createSeparatorActionItem();
        actionList << Kicker::editApplicationAction(m_service);
        actionList << Kicker::appstreamActions(m_service);
    }

    if (appletInterface) {
        QQmlPropertyMap *appletConfig = qobject_cast<QQmlPropertyMap *>(appletInterface->property("configuration").value<QObject *>());

        if (appletConfig && appletConfig->contains(QLatin1String("hiddenApplications")) && qobject_cast<AppsModel *>(m_owner)) {
            const QStringList &hiddenApps = appletConfig->value(QLatin1String("hiddenApplications")).toStringList();

            if (!hiddenApps.contains(m_service->menuId())) {
                QVariantMap hideAction = Kicker::createActionItem(i18n("Hide Application"), QStringLiteral("view-hidden"), QStringLiteral("hideApplication"));
                actionList << hideAction;
            }
        }
    }

    return actionList;
}

bool AppEntry::run(const QString &actionId, const QVariant &argument)
{
    if (!m_service->isValid()) {
        return false;
    }

    if (actionId.isEmpty()) {
        quint32 timeStamp = 0;

#if HAVE_X11
        if (QX11Info::isPlatformX11()) {
            timeStamp = QX11Info::appUserTime();
        }
#endif

        auto *job = new KIO::ApplicationLauncherJob(m_service);
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
        if (KWindowSystem::isPlatformX11()) {
            job->setStartupId(KStartupInfo::createNewStartupIdForTimestamp(timeStamp));
        }
        job->start();

        KActivities::ResourceInstance::notifyAccessed(QUrl(QStringLiteral("applications:") + m_service->storageId()), QStringLiteral("org.kde.plasma.kicker"));

        return true;
    }

    QObject *appletInterface = m_owner->rootModel()->property("appletInterface").value<QObject *>();

    if (Kicker::handleAddLauncherAction(actionId, appletInterface, m_service)) {
        return false; // We don't want to close Kicker, BUG: 390585
    } else if (Kicker::handleEditApplicationAction(actionId, m_service)) {
        return true;
    } else if (Kicker::handleAppstreamActions(actionId, argument)) {
        return true;
    } else if (actionId == QLatin1String("_kicker_jumpListAction")) {
        auto job = new KIO::CommandLauncherJob(argument.toString());
        job->setDesktopName(m_service->entryPath());
        job->setIcon(m_service->icon());
        return job->exec();
    }

    return Kicker::handleRecentDocumentAction(m_service, actionId, argument);
}

QString AppEntry::nameFromService(const KService::Ptr service, NameFormat nameFormat)
{
    const QString &name = service->name();
    QString genericName = service->genericName();

    if (genericName.isEmpty()) {
        genericName = service->comment();
    }

    if (nameFormat == NameOnly || genericName.isEmpty() || name == genericName) {
        return name;
    } else if (nameFormat == GenericNameOnly) {
        return genericName;
    } else if (nameFormat == NameAndGenericName) {
        return i18nc("App name (Generic name)", "%1 (%2)", name, genericName);
    } else {
        return i18nc("Generic name (App name)", "%1 (%2)", genericName, name);
    }
}

KService::Ptr AppEntry::defaultAppByName(const QString &name)
{
    if (name == QLatin1String("browser")) {
        KConfigGroup config(KSharedConfig::openConfig(), "General");
        QString browser = config.readPathEntry("BrowserApplication", QString());

        if (browser.isEmpty()) {
            return KApplicationTrader::preferredService(QLatin1String("text/html"));
        } else if (browser.startsWith(QLatin1Char('!'))) {
            browser.remove(0, 1);
        }

        return KService::serviceByStorageId(browser);
    }

    return KService::Ptr();
}

AppEntry::~AppEntry()
{
    if (m_con) {
        QObject::disconnect(m_con);
    }
}

AppGroupEntry::AppGroupEntry(AppsModel *parentModel,
                             KServiceGroup::Ptr group,
                             bool paginate,
                             int pageSize,
                             bool flat,
                             bool sorted,
                             bool separators,
                             int appNameFormat)
    : AbstractGroupEntry(parentModel)
    , m_group(group)
{
    AppsModel *model = new AppsModel(group->entryPath(), paginate, pageSize, flat, sorted, separators, parentModel);
    model->setAppNameFormat(appNameFormat);
    m_childModel = model;

    QObject::connect(parentModel, &AppsModel::cleared, model, &AppsModel::deleteLater);

    QObject::connect(model, &AppsModel::countChanged, [parentModel, this] {
        if (parentModel) {
            parentModel->entryChanged(this);
        }
    });

    QObject::connect(model, &AppsModel::hiddenEntriesChanged, [parentModel, this] {
        if (parentModel) {
            parentModel->entryChanged(this);
        }
    });
}

QIcon AppGroupEntry::icon() const
{
    if (m_icon.isNull()) {
        m_icon = QIcon::fromTheme(m_group->icon(), QIcon::fromTheme(QStringLiteral("unknown")));
    }
    return m_icon;
}

QString AppGroupEntry::name() const
{
    return m_group->caption();
}

bool AppGroupEntry::hasChildren() const
{
    return m_childModel && m_childModel->count() > 0;
}

AbstractModel *AppGroupEntry::childModel() const
{
    return m_childModel;
}
