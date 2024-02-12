/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "menu.h"

#include <QAction>
#include <QCheckBox>
#include <QDBusPendingReply>
#include <QVBoxLayout>

#include <KAuthorized>
#include <KGlobalAccel>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KService>
#include <KTerminalLauncherJob>
#include <PlasmaActivities/Consumer>
#include <QDebug>
#include <QIcon>
#include <QQmlListProperty>

#include <Plasma/Containment>
#include <Plasma/Corona>

#include "krunner_interface.h"
#include "kworkspace.h"
#include <sessionmanagement.h>

using namespace Qt::StringLiterals;

ContextMenu::ContextMenu(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
    , m_session(new SessionManagement(this))
{
}

ContextMenu::~ContextMenu()
{
}

void ContextMenu::restore(const KConfigGroup &config)
{
    Plasma::Containment *c = containment();
    Q_ASSERT(c);

    m_actions.clear();
    m_actionOrder.clear();
    QHash<QString, bool> actions;
    QSet<QString> disabled;

    // clang-format off
    // because it really wants to mangle this nice aligned list
    if (c->containmentType() == Plasma::Containment::Panel || c->containmentType() == Plasma::Containment::CustomPanel) {
        m_actionOrder << QStringLiteral("add widgets")
                      << QStringLiteral("_context")
                      << QStringLiteral("configure")
                      << QStringLiteral("remove");
    } else {
        actions.insert(QStringLiteral("configure shortcuts"), false);
        m_actionOrder << QStringLiteral("configure")
                      << QStringLiteral("_display_settings")
                      << QStringLiteral("configure shortcuts")
                      << QStringLiteral("_sep1")
                      << QStringLiteral("_context")
                      << QStringLiteral("_open_terminal")
                      << QStringLiteral("_run_command")
                      << QStringLiteral("add widgets")
                      << QStringLiteral("_add panel")
                      << QStringLiteral("manage activities")
                      << QStringLiteral("remove")
                      << QStringLiteral("edit mode")
                      << QStringLiteral("_sep2")
                      << QStringLiteral("_lock_screen")
                      << QStringLiteral("_logout")
                      << QStringLiteral("_sep3")
                      << QStringLiteral("_wallpaper");
        disabled.insert(QStringLiteral("add widgets"));
        disabled.insert(QStringLiteral("_add panel"));
        disabled.insert(QStringLiteral("configure shortcuts"));
        disabled.insert(QStringLiteral("_open_terminal"));
        disabled.insert(QStringLiteral("_run_command"));
        disabled.insert(QStringLiteral("_lock_screen"));
        disabled.insert(QStringLiteral("_logout"));
    }
    // clang-format on

    for (const QString &name : std::as_const(m_actionOrder)) {
        actions.insert(name, !disabled.contains(name));
    }

    QHashIterator<QString, bool> it(actions);
    while (it.hasNext()) {
        it.next();
        m_actions.insert(it.key(), config.readEntry(it.key(), it.value()));
    }

    // everything below should only happen once, so check for it
    if (!m_runCommandAction) {
        m_runCommandAction = new QAction(i18nc("plasma_containmentactions_contextmenu", "Show KRunner"), this);
        m_runCommandAction->setIcon(QIcon::fromTheme(QStringLiteral("plasma-search")));
        m_runCommandAction->setShortcut(KGlobalAccel::self()->globalShortcut(QStringLiteral("krunner.desktop"), QStringLiteral("_launch")).value(0));
        connect(m_runCommandAction, &QAction::triggered, this, &ContextMenu::runCommand);

        m_openTerminalAction = new QAction(i18n("Open Terminal"), this);
        m_openTerminalAction->setIcon(QIcon::fromTheme("utilities-terminal"));
        connect(m_openTerminalAction, &QAction::triggered, this, &ContextMenu::openTerminal);

        m_lockScreenAction = new QAction(i18nc("plasma_containmentactions_contextmenu", "Lock Screen"), this);
        m_lockScreenAction->setIcon(QIcon::fromTheme(QStringLiteral("system-lock-screen")));
        m_lockScreenAction->setShortcut(KGlobalAccel::self()->globalShortcut(QStringLiteral("ksmserver"), QStringLiteral("Lock Session")).value(0));
        m_lockScreenAction->setEnabled(m_session->canLock());
        connect(m_session, &SessionManagement::canLockChanged, this, [this]() {
            m_lockScreenAction->setEnabled(m_session->canLock());
        });
        connect(m_lockScreenAction, &QAction::triggered, m_session, &SessionManagement::lock);

        m_logoutAction = new QAction(i18nc("plasma_containmentactions_contextmenu", "Leave…"), this);
        m_logoutAction->setIcon(QIcon::fromTheme(QStringLiteral("system-log-out")));
        m_logoutAction->setShortcut(KGlobalAccel::self()->globalShortcut(QStringLiteral("ksmserver"), QStringLiteral("Log Out")).value(0));
        m_logoutAction->setEnabled(m_session->canLogout());
        connect(m_session, &SessionManagement::canLogoutChanged, this, [this]() {
            m_logoutAction->setEnabled(m_session->canLogout());
        });
        connect(m_logoutAction, &QAction::triggered, this, &ContextMenu::startLogout);

        m_configureDisplaysAction = new QAction(i18nc("plasma_containmentactions_contextmenu", "Configure Display Settings…"), this);
        m_configureDisplaysAction->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-display")));
        connect(m_configureDisplaysAction, &QAction::triggered, this, &ContextMenu::configureDisplays);

        m_separator1 = new QAction(this);
        m_separator1->setSeparator(true);
        m_separator2 = new QAction(this);
        m_separator2->setSeparator(true);
        m_separator3 = new QAction(this);
        m_separator3->setSeparator(true);
    }
}

QList<QAction *> ContextMenu::contextualActions()
{
    Plasma::Containment *cont = containment();
    Q_ASSERT(cont);
    QList<QAction *> actions;
    for (const QString &name : std::as_const(m_actionOrder)) {
        if (!m_actions.value(name)) {
            continue;
        }

        if (name == QLatin1String("_context")) {
            actions << cont->contextualActions();
        }
        if (name == QLatin1String("_wallpaper")) {
            if (!cont->wallpaperPlugin().isEmpty()) {
                QObject *wallpaperGraphicsObject = cont->property("wallpaperGraphicsObject").value<QObject *>();
                if (wallpaperGraphicsObject) {
                    QQmlListProperty<QAction> l = wallpaperGraphicsObject->property("contextualActions").value<QQmlListProperty<QAction>>();
                    int nActions = l.count(&l);
                    for (int i = 0; i < nActions; ++i) {
                        actions << l.at(&l, i);
                    }
                }
            }
        } else if (QAction *a = action(name)) {
            // Bug 364292: show "Remove this Panel" option only when panelcontroller is opened
            if (name != QLatin1String("remove") || cont->isUserConfiguring()
                || (cont->containmentType() != Plasma::Containment::Panel && cont->containmentType() != Plasma::Containment::CustomPanel
                    && cont->corona()->immutability() != Plasma::Types::Mutable)) {
                actions << a;
            }
        }
    }

    return actions;
}

QAction *ContextMenu::action(const QString &name)
{
    Plasma::Containment *c = containment();
    Q_ASSERT(c);
    if (name == QLatin1String("_sep1")) {
        return m_separator1;
    } else if (name == QLatin1String("_sep2")) {
        return m_separator2;
    } else if (name == QLatin1String("_sep3")) {
        return m_separator3;
    } else if (name == QLatin1String("_add panel")) {
        if (c->corona() && c->corona()->immutability() == Plasma::Types::Mutable) {
            return c->corona()->action(QStringLiteral("add panel"));
        }
    } else if (name == QLatin1String("_run_command")) {
        if (KAuthorized::authorizeAction(QStringLiteral("run_command")) && KAuthorized::authorize(QStringLiteral("run_command"))) {
            return m_runCommandAction;
        }
    } else if (name == QLatin1String("_open_terminal")) {
        if (KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
            return m_openTerminalAction;
        }
    } else if (name == QLatin1String("_lock_screen")) {
        if (KAuthorized::authorizeAction(QStringLiteral("lock_screen"))) {
            return m_lockScreenAction;
        }
    } else if (name == QLatin1String("_logout")) {
        if (KAuthorized::authorize(QStringLiteral("logout"))) {
            return m_logoutAction;
        }
    } else if (name == QLatin1String("_display_settings")) {
        if (KAuthorized::authorizeControlModule(QStringLiteral("kcm_kscreen.desktop")) && KService::serviceByStorageId(QStringLiteral("kcm_kscreen"))) {
            return m_configureDisplaysAction;
        }
    } else if (name == QLatin1String("edit mode")) {
        if (c->corona()) {
            return c->corona()->action(QStringLiteral("edit mode"));
        }
    } else if (name == QLatin1String("manage activities")) {
        if (c->corona()) {
            // Don't show the action if there's only one activity since in this
            // case it's clear that the user doesn't use activities
            if (KActivities::Consumer().activities().length() == 1) {
                return nullptr;
            }

            return c->corona()->action(QStringLiteral("manage activities"));
        }
    } else {
        // FIXME: remove action: make removal of current activity possible
        return c->internalAction(name);
    }
    return nullptr;
}

void ContextMenu::openTerminal()
{
    if (!KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
        return;
    }
    auto job = new KTerminalLauncherJob(QString());
    job->setWorkingDirectory(QDir::homePath());
    job->start();
}

void ContextMenu::runCommand()
{
    if (!KAuthorized::authorizeAction(QStringLiteral("run_command"))) {
        return;
    }

    QString interface(QStringLiteral("org.kde.krunner"));
    org::kde::krunner::App krunner(interface, QStringLiteral("/App"), QDBusConnection::sessionBus());
    krunner.display();
}

void ContextMenu::startLogout()
{
    KConfig config(u"ksmserverrc"_s);
    const auto group = config.group(u"General"_s);
    switch (group.readEntry("shutdownType", int(KWorkSpace::ShutdownTypeNone))) {
    case int(KWorkSpace::ShutdownTypeHalt):
        m_session->requestShutdown();
        break;
    case int(KWorkSpace::ShutdownTypeReboot):
        m_session->requestReboot();
        break;
    default:
        m_session->requestLogout();
        break;
    }
}

void ContextMenu::configureDisplays()
{
    if (auto service = KService::serviceByDesktopName(QStringLiteral("kcm_kscreen"))) {
        auto job = new KIO::ApplicationLauncherJob(service);
        job->start();
    }
}

QWidget *ContextMenu::createConfigurationInterface(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    QVBoxLayout *lay = new QVBoxLayout();
    widget->setLayout(lay);
    widget->setWindowTitle(i18nc("plasma_containmentactions_contextmenu", "Configure Contextual Menu Plugin"));
    m_buttons = new QButtonGroup(widget);
    m_buttons->setExclusive(false);

    foreach (const QString &name, m_actionOrder) {
        QCheckBox *item = nullptr;

        if (name == QLatin1String("_context")) {
            item = new QCheckBox(widget);
            // FIXME better text
            item->setText(i18nc("plasma_containmentactions_contextmenu", "[Other Actions]"));
        } else if (name == QLatin1String("_wallpaper")) {
            item = new QCheckBox(widget);
            item->setText(i18nc("plasma_containmentactions_contextmenu", "Wallpaper Actions"));
            item->setIcon(QIcon::fromTheme(QStringLiteral("user-desktop")));
        } else if (name == QLatin1String("_sep1") || name == QLatin1String("_sep2") || name == QLatin1String("_sep3")) {
            item = new QCheckBox(widget);
            item->setText(i18nc("plasma_containmentactions_contextmenu", "[Separator]"));
        } else {
            QAction *a = action(name);
            if (a) {
                item = new QCheckBox(widget);
                item->setText(a->text());
                item->setIcon(a->icon());
            }
        }

        if (item) {
            item->setChecked(m_actions.value(name));
            item->setProperty("actionName", name);
            lay->addWidget(item);
            m_buttons->addButton(item);
        }
    }

    return widget;
}

void ContextMenu::configurationAccepted()
{
    QList<QAbstractButton *> buttons = m_buttons->buttons();
    QListIterator<QAbstractButton *> it(buttons);
    while (it.hasNext()) {
        QAbstractButton *b = it.next();
        if (b) {
            m_actions.insert(b->property("actionName").toString(), b->isChecked());
        }
    }
}

void ContextMenu::save(KConfigGroup &config)
{
    QHashIterator<QString, bool> it(m_actions);
    while (it.hasNext()) {
        it.next();
        config.writeEntry(it.key(), it.value());
    }
}

K_PLUGIN_CLASS_WITH_JSON(ContextMenu, "plasma-containmentactions-contextmenu.json")

#include "menu.moc"
#include "moc_menu.cpp"
