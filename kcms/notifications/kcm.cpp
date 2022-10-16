/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "kcm.h"

#include <QAction>
#include <QCommandLineParser>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWindow>

#include <KConfigGroup>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KNotifyConfigWidget>
#include <KPluginFactory>

#include <algorithm>

#include "notificationsdata.h"

#include <libnotificationmanager/badgesettings.h>
#include <libnotificationmanager/behaviorsettings.h>
#include <libnotificationmanager/donotdisturbsettings.h>
#include <libnotificationmanager/jobsettings.h>
#include <libnotificationmanager/notificationsettings.h>

K_PLUGIN_FACTORY_WITH_JSON(KCMNotificationsFactory, "kcm_notifications.json", registerPlugin<KCMNotifications>(); registerPlugin<NotificationsData>();)

KCMNotifications::KCMNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_sourcesModel(new SourcesModel(this))
    , m_filteredModel(new FilterProxyModel(this))
    , m_data(new NotificationsData(this))
    , m_toggleDoNotDisturbAction(new QAction(this))
{
    const char uri[] = "org.kde.private.kcms.notifications";
    qmlRegisterUncreatableType<SourcesModel>(uri, 1, 0, "SourcesModel", QStringLiteral("Cannot create instances of SourcesModel"));

    qmlRegisterAnonymousType<FilterProxyModel>("FilterProxyModel", 1);
    qmlRegisterAnonymousType<QKeySequence>("QKeySequence", 1);
    qmlRegisterAnonymousType<NotificationManager::DoNotDisturbSettings>("DoNotDisturbSettings", 1);
    qmlRegisterAnonymousType<NotificationManager::NotificationSettings>("NotificationSettings", 1);
    qmlRegisterAnonymousType<NotificationManager::JobSettings>("JobSettings", 1);
    qmlRegisterAnonymousType<NotificationManager::BadgeSettings>("BadgeSettings", 1);
    qmlRegisterAnonymousType<NotificationManager::BehaviorSettings>("BehaviorSettings", 1);
    qmlProtectModule(uri, 1);

    m_filteredModel->setSourceModel(m_sourcesModel);

    // for KGlobalAccel...
    // keep in sync with globalshortcuts.cpp in notification plasmoid!
    m_toggleDoNotDisturbAction->setObjectName(QStringLiteral("toggle do not disturb"));
    m_toggleDoNotDisturbAction->setProperty("componentName", QStringLiteral("plasmashell"));
    m_toggleDoNotDisturbAction->setText(i18n("Toggle do not disturb"));
    m_toggleDoNotDisturbAction->setIcon(QIcon::fromTheme(QStringLiteral("notifications-disabled")));

    QStringList stringArgs;
    stringArgs.reserve(args.count() + 1);
    // need to add a fake argv[0] for QCommandLineParser
    stringArgs.append(QStringLiteral("kcm_notifications"));
    for (const QVariant &arg : args) {
        stringArgs.append(arg.toString());
    }

    QCommandLineParser parser;

    QCommandLineOption desktopEntryOption(QStringLiteral("desktop-entry"), QString(), QStringLiteral("desktop-entry"));
    parser.addOption(desktopEntryOption);
    QCommandLineOption notifyRcNameOption(QStringLiteral("notifyrc"), QString(), QStringLiteral("notifyrcname"));
    parser.addOption(notifyRcNameOption);
    QCommandLineOption eventIdOption(QStringLiteral("event-id"), QString(), QStringLiteral("event-id"));
    parser.addOption(eventIdOption);

    parser.parse(stringArgs);

    setInitialDesktopEntry(parser.value(desktopEntryOption));
    setInitialNotifyRcName(parser.value(notifyRcNameOption));
    setInitialEventId(parser.value(eventIdOption));

    connect(this, &KCMNotifications::toggleDoNotDisturbShortcutChanged, this, &KCMNotifications::settingsChanged);
    connect(this, &KCMNotifications::defaultsIndicatorsVisibleChanged, this, &KCMNotifications::onDefaultsIndicatorsVisibleChanged);
}

KCMNotifications::~KCMNotifications()
{
}

SourcesModel *KCMNotifications::sourcesModel() const
{
    return m_sourcesModel;
}

FilterProxyModel *KCMNotifications::filteredModel() const
{
    return m_filteredModel;
}

NotificationManager::DoNotDisturbSettings *KCMNotifications::dndSettings() const
{
    return m_data->dndSettings();
}

NotificationManager::NotificationSettings *KCMNotifications::notificationSettings() const
{
    return m_data->notificationSettings();
}

NotificationManager::JobSettings *KCMNotifications::jobSettings() const
{
    return m_data->jobSettings();
}

NotificationManager::BadgeSettings *KCMNotifications::badgeSettings() const
{
    return m_data->badgeSettings();
}

QKeySequence KCMNotifications::toggleDoNotDisturbShortcut() const
{
    return m_toggleDoNotDisturbShortcut;
}

void KCMNotifications::setToggleDoNotDisturbShortcut(const QKeySequence &shortcut)
{
    if (m_toggleDoNotDisturbShortcut == shortcut) {
        return;
    }

    m_toggleDoNotDisturbShortcut = shortcut;
    m_toggleDoNotDisturbShortcutDirty = true;
    Q_EMIT toggleDoNotDisturbShortcutChanged();
}

QString KCMNotifications::initialDesktopEntry() const
{
    return m_initialDesktopEntry;
}

void KCMNotifications::setInitialDesktopEntry(const QString &desktopEntry)
{
    if (m_initialDesktopEntry != desktopEntry) {
        m_initialDesktopEntry = desktopEntry;
        Q_EMIT initialDesktopEntryChanged();
    }
}

QString KCMNotifications::initialNotifyRcName() const
{
    return m_initialNotifyRcName;
}

void KCMNotifications::setInitialNotifyRcName(const QString &notifyRcName)
{
    if (m_initialNotifyRcName != notifyRcName) {
        m_initialNotifyRcName = notifyRcName;
        Q_EMIT initialNotifyRcNameChanged();
    }
}

QString KCMNotifications::initialEventId() const
{
    return m_initialEventId;
}

void KCMNotifications::setInitialEventId(const QString &eventId)
{
    if (m_initialEventId != eventId) {
        m_initialEventId = eventId;
        Q_EMIT initialEventIdChanged();
    }
}

void KCMNotifications::configureEvents(const QString &notifyRcName, const QString &eventId, QQuickItem *ctx)
{
    // We're not using KNotifyConfigWidget::configure here as we want to handle the
    // saving ourself (so we Apply with all other KCM settings) but there's no way
    // to access the config object :(
    // We also need access to the QDialog so we can set the KCM as transient parent.

    QDialog *dialog = new QDialog(nullptr);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(i18n("Configure Notifications"));

    if (ctx && ctx->window()) {
        dialog->winId(); // so it creates windowHandle
        dialog->windowHandle()->setTransientParent(QQuickRenderControl::renderWindowFor(ctx->window()));
        dialog->setModal(true);
    }

    KNotifyConfigWidget *w = new KNotifyConfigWidget(dialog);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(dialog);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(w);
    layout->addWidget(buttonBox);
    dialog->setLayout(layout);

    // TODO we should only save settings when clicking Apply in the main UI
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, w, &KNotifyConfigWidget::save);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, w, &KNotifyConfigWidget::save);
    connect(w, &KNotifyConfigWidget::changed, buttonBox->button(QDialogButtonBox::Apply), &QPushButton::setEnabled);

    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    w->setApplication(notifyRcName);
    w->selectEvent(eventId);

    dialog->show();
}

NotificationManager::BehaviorSettings *KCMNotifications::behaviorSettings(const QModelIndex &index)
{
    if (!index.isValid()) {
        return nullptr;
    }
    return m_data->behaviorSettings(index.row());
}

bool KCMNotifications::isDefaultsBehaviorSettings() const
{
    return m_data->isDefaultsBehaviorSettings();
}

void KCMNotifications::load()
{
    ManagedConfigModule::load();

    bool firstLoad = m_firstLoad;
    if (m_firstLoad) {
        m_firstLoad = false;
        m_sourcesModel->load();

        for (int i = 0; i < m_sourcesModel->rowCount(); ++i) {
            const QModelIndex index = m_sourcesModel->index(i, 0);
            if (!index.isValid()) {
                continue;
            }

            QString typeName;
            QString groupName;
            if (m_sourcesModel->data(index, SourcesModel::SourceTypeRole) == SourcesModel::ApplicationType) {
                typeName = QStringLiteral("Applications");
                groupName = m_sourcesModel->data(index, SourcesModel::DesktopEntryRole).toString();
            } else {
                typeName = QStringLiteral("Services");
                groupName = m_sourcesModel->data(index, SourcesModel::NotifyRcNameRole).toString();
            }
            auto *toAdd = new NotificationManager::BehaviorSettings(typeName, groupName, this);
            m_data->insertBehaviorSettings(index.row(), toAdd);
            createConnections(toAdd, index);
        }
    }

    m_data->loadBehaviorSettings();

    const QKeySequence toggleDoNotDisturbShortcut =
        KGlobalAccel::self()
            ->globalShortcut(m_toggleDoNotDisturbAction->property("componentName").toString(), m_toggleDoNotDisturbAction->objectName())
            .value(0);

    if (m_toggleDoNotDisturbShortcut != toggleDoNotDisturbShortcut) {
        m_toggleDoNotDisturbShortcut = toggleDoNotDisturbShortcut;
        Q_EMIT toggleDoNotDisturbShortcutChanged();
    }

    m_toggleDoNotDisturbShortcutDirty = false;
    if (firstLoad) {
        Q_EMIT firstLoadDone();
    }
}

void KCMNotifications::save()
{
    ManagedConfigModule::save();
    m_data->saveBehaviorSettings();

    if (m_toggleDoNotDisturbShortcutDirty) {
        // KeySequenceItem will already have checked whether the shortcut is available
        KGlobalAccel::self()->setShortcut(m_toggleDoNotDisturbAction, {m_toggleDoNotDisturbShortcut}, KGlobalAccel::NoAutoloading);
    }
}

void KCMNotifications::defaults()
{
    ManagedConfigModule::defaults();
    m_data->defaultsBehaviorSettings();

    setToggleDoNotDisturbShortcut(QKeySequence());
}

void KCMNotifications::onDefaultsIndicatorsVisibleChanged()
{
    for (int i = 0; i < m_sourcesModel->rowCount(); ++i) {
        const QModelIndex index = m_sourcesModel->index(i, 0);
        updateModelIsDefaultStatus(index);
    }
}

void KCMNotifications::updateModelIsDefaultStatus(const QModelIndex &index)
{
    if (index.isValid()) {
        m_sourcesModel->setData(index, behaviorSettings(index)->isDefaults(), SourcesModel::IsDefaultRole);
        Q_EMIT isDefaultsBehaviorSettingsChanged();
    }
}

bool KCMNotifications::isSaveNeeded() const
{
    return m_toggleDoNotDisturbShortcutDirty || m_data->isSaveNeededBehaviorSettings();
}

bool KCMNotifications::isDefaults() const
{
    return m_data->isDefaultsBehaviorSettings();
}

void KCMNotifications::createConnections(NotificationManager::BehaviorSettings *settings, const QModelIndex &index)
{
    connect(settings, &NotificationManager::BehaviorSettings::ShowPopupsChanged, this, &KCMNotifications::settingsChanged);
    connect(settings, &NotificationManager::BehaviorSettings::ShowPopupsInDndModeChanged, this, &KCMNotifications::settingsChanged);
    connect(settings, &NotificationManager::BehaviorSettings::ShowInHistoryChanged, this, &KCMNotifications::settingsChanged);
    connect(settings, &NotificationManager::BehaviorSettings::ShowBadgesChanged, this, &KCMNotifications::settingsChanged);

    connect(settings, &NotificationManager::BehaviorSettings::ShowPopupsChanged, this, [this, index] {
        updateModelIsDefaultStatus(index);
    });
    connect(settings, &NotificationManager::BehaviorSettings::ShowPopupsInDndModeChanged, this, [this, index] {
        updateModelIsDefaultStatus(index);
    });
    connect(settings, &NotificationManager::BehaviorSettings::ShowInHistoryChanged, this, [this, index] {
        updateModelIsDefaultStatus(index);
    });
    connect(settings, &NotificationManager::BehaviorSettings::ShowBadgesChanged, this, [this, index] {
        updateModelIsDefaultStatus(index);
    });
}

#include "kcm.moc"
