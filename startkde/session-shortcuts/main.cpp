/*
 SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KActionCollection>
#include <KDEDModule>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KPluginFactory>

#include <sessionmanagement.h>

class SessionShortcutsModule : public KDEDModule
{
    Q_OBJECT
public:
    SessionShortcutsModule(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
};

K_PLUGIN_CLASS_WITH_JSON(SessionShortcutsModule, "sessionshortcuts.json");

SessionShortcutsModule::SessionShortcutsModule(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : KDEDModule(parent)
{
    Q_UNUSED(args)
    Q_UNUSED(metaData)
    SessionManagement *sessionManagement = new SessionManagement(this);

    KActionCollection *actionCollection = new KActionCollection(this);
    actionCollection->setComponentDisplayName(i18n("Session Management"));
    actionCollection->setComponentName(QStringLiteral("ksmserver")); // for migration purposes
    QAction *a;

    // "With confirmation" actions
    a = actionCollection->addAction(QStringLiteral("Log Out"));
    a->setText(i18n("Log Out"));
    KGlobalAccel::self()->setGlobalShortcut(a, QList<QKeySequence>() << (Qt::ALT | Qt::CTRL | Qt::Key_Delete));
    connect(a, &QAction::triggered, this, [sessionManagement]() {
        sessionManagement->requestLogout(SessionManagement::ConfirmationMode::ForcePrompt);
    });
    a = actionCollection->addAction(QStringLiteral("Shut Down"));
    a->setText(i18n("Shut Down"));
    KGlobalAccel::self()->setGlobalShortcut(a, QKeySequence());
    connect(a, &QAction::triggered, this, [sessionManagement]() {
        sessionManagement->requestShutdown(SessionManagement::ConfirmationMode::ForcePrompt);
    });
    a = actionCollection->addAction(QStringLiteral("Reboot"));
    a->setText(i18n("Reboot"));
    KGlobalAccel::self()->setGlobalShortcut(a, QKeySequence());
    connect(a, &QAction::triggered, this, [sessionManagement]() {
        sessionManagement->requestReboot(SessionManagement::ConfirmationMode::ForcePrompt);
    });

    // "Without confirmation" actions
    a = actionCollection->addAction(QStringLiteral("Log Out Without Confirmation"));
    a->setText(i18n("Log Out Without Confirmation"));
    KGlobalAccel::self()->setGlobalShortcut(a, QKeySequence());
    connect(a, &QAction::triggered, this, [sessionManagement]() {
        sessionManagement->requestLogout(SessionManagement::ConfirmationMode::Skip);
    });
    a = actionCollection->addAction(QStringLiteral("Halt Without Confirmation"));
    a->setText(i18n("Shut Down Without Confirmation"));
    KGlobalAccel::self()->setGlobalShortcut(a, QKeySequence());
    connect(a, &QAction::triggered, this, [sessionManagement]() {
        sessionManagement->requestShutdown(SessionManagement::ConfirmationMode::Skip);
    });
    a = actionCollection->addAction(QStringLiteral("Reboot Without Confirmation"));
    a->setText(i18n("Reboot Without Confirmation"));
    KGlobalAccel::self()->setGlobalShortcut(a, QKeySequence());
    connect(a, &QAction::triggered, this, [sessionManagement]() {
        sessionManagement->requestReboot(SessionManagement::ConfirmationMode::Skip);
    });
}

#include "main.moc"
