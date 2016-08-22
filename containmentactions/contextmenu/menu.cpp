/*
 *   Copyright 2009 by Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "menu.h"

#include <QAction>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QDBusPendingReply>

#include <KActionCollection>
#include <KAuthorized>
#include <QDebug>
#include <QIcon>
#include <KGlobalAccel>
#include <KLocalizedString>

#include <Plasma/Containment>
#include <Plasma/Corona>

#include "kworkspace.h"
#include "krunner_interface.h"
#include "screensaver_interface.h"

#ifdef Q_OS_WIN
#define _WIN32_WINNT 0x0500 // require NT 5.0 (win 2k pro)
#include <windows.h>
#endif // Q_OS_WIN

ContextMenu::ContextMenu(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args),
      m_runCommandAction(0),
      m_lockScreenAction(0),
      m_logoutAction(0),
      m_separator1(0),
      m_separator2(0),
      m_separator3(0),
      m_buttons(0)
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

    if (c->containmentType() == Plasma::Types::PanelContainment ||
        c->containmentType() == Plasma::Types::CustomPanelContainment) {
        m_actionOrder << QStringLiteral("add widgets") << QStringLiteral("_add panel") << QStringLiteral("lock widgets") << QStringLiteral("_context") << QStringLiteral("configure") << QStringLiteral("remove");
    } else {
        actions.insert(QStringLiteral("configure shortcuts"), false);
        m_actionOrder << QStringLiteral("_context") << QStringLiteral("_run_command") << QStringLiteral("add widgets") << QStringLiteral("_add panel")
                      << QStringLiteral("manage activities") << QStringLiteral("remove") << QStringLiteral("lock widgets") << QStringLiteral("_sep1")
                      <<QStringLiteral("_lock_screen") << QStringLiteral("_logout") << QStringLiteral("_sep2") << QStringLiteral("run associated application") << QStringLiteral("configure")
                      << QStringLiteral("configure shortcuts") << QStringLiteral("_sep3") << QStringLiteral("_wallpaper");
        disabled.insert(QStringLiteral("configure shortcuts"));
    }

    foreach (const QString &name, m_actionOrder) {
        actions.insert(name, !disabled.contains(name));
    }

    QHashIterator<QString, bool> it(actions);
    while (it.hasNext()) {
        it.next();
        m_actions.insert(it.key(), config.readEntry(it.key(), it.value()));
    }

    // everything below should only happen once, so check for it
    if (!m_runCommandAction) {
        m_runCommandAction = new QAction(i18nc("plasma_containmentactions_contextmenu", "Run Command..."), this);
        m_runCommandAction->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
        m_runCommandAction->setShortcut(KGlobalAccel::self()->globalShortcut(QStringLiteral("krunner"), QStringLiteral("run command")).value(0));
        connect(m_runCommandAction, &QAction::triggered, this, &ContextMenu::runCommand);

        m_lockScreenAction = new QAction(i18nc("plasma_containmentactions_contextmenu", "Lock Screen"), this);
        m_lockScreenAction->setIcon(QIcon::fromTheme(QStringLiteral("system-lock-screen")));
        m_lockScreenAction->setShortcut(KGlobalAccel::self()->globalShortcut(QStringLiteral("ksmserver"), QStringLiteral("Lock Session")).value(0));
        connect(m_lockScreenAction, &QAction::triggered, this, &ContextMenu::lockScreen);

        m_logoutAction = new QAction(i18nc("plasma_containmentactions_contextmenu", "Leave..."), this);
        m_logoutAction->setIcon(QIcon::fromTheme(QStringLiteral("system-log-out")));
        m_logoutAction->setShortcut(KGlobalAccel::self()->globalShortcut(QStringLiteral("ksmserver"), QStringLiteral("Log Out")).value(0));
        connect(m_logoutAction, &QAction::triggered, this, &ContextMenu::startLogout);

        m_separator1 = new QAction(this);
        m_separator1->setSeparator(true);
        m_separator2 = new QAction(this);
        m_separator2->setSeparator(true);
        m_separator3 = new QAction(this);
        m_separator3->setSeparator(true);
    }
}

QList<QAction*> ContextMenu::contextualActions()
{
    Plasma::Containment *c = containment();
    Q_ASSERT(c);
    QList<QAction*> actions;
    foreach (const QString &name, m_actionOrder) {
        if (!m_actions.value(name)) {
            continue;
        }

        if (name == QLatin1String("_context")) {
            actions << c->contextualActions();
        } if (name == QLatin1String("_wallpaper")) {
            if (!c->wallpaper().isEmpty()) {
                QObject *wallpaperGraphicsObject = c->property("wallpaperGraphicsObject").value<QObject *>();
                if (wallpaperGraphicsObject) {
                    actions << wallpaperGraphicsObject->property("contextualActions").value<QList<QAction *> >();
                }
            }
        } else if (QAction *a = action(name)) {
            // Bug 364292: show "Remove this Panel" option only when panelcontroller is opened
            if (name != QLatin1String("remove") || c->isUserConfiguring() ||
                (c->containmentType() != Plasma::Types::PanelContainment
                 && c->containmentType() != Plasma::Types::CustomPanelContainment)) {

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
            return c->corona()->actions()->action(QStringLiteral("add panel"));
        }
    } else if (name == QLatin1String("_run_command")) {
        if (KAuthorized::authorizeAction(QStringLiteral("run_command"))) {
            return m_runCommandAction;
        }
    } else if (name == QLatin1String("_lock_screen")) {
        if (KAuthorized::authorizeAction(QStringLiteral("lock_screen"))) {
            return m_lockScreenAction;
        }
    } else if (name == QLatin1String("_logout")) {
        if (KAuthorized::authorizeAction(QStringLiteral("logout"))) {
            return m_logoutAction;
        }
    } else if (name == QLatin1String("lock widgets")) {
        if (c->corona()) {
            return c->corona()->actions()->action(QStringLiteral("lock widgets"));
        }
    } else if (name == QLatin1String("manage activities")) {
        if (c->corona()) {
            return c->corona()->actions()->action(QStringLiteral("manage activities"));
        }
    } else {
        //FIXME: remove action: make removal of current activity possible
        return c->actions()->action(name);
    }
    return 0;
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

void ContextMenu::lockScreen()
{
    if (!KAuthorized::authorizeAction(QStringLiteral("lock_screen"))) {
        return;
    }

#ifndef Q_OS_WIN
    QString interface(QStringLiteral("org.freedesktop.ScreenSaver"));
    org::freedesktop::ScreenSaver screensaver(interface, QStringLiteral("/ScreenSaver"),
                                              QDBusConnection::sessionBus());
    if (screensaver.isValid()) {
        screensaver.Lock();
    }
#else
    LockWorkStation();
#endif // !Q_OS_WIN
}

void ContextMenu::startLogout()
{
    // this short delay is due to two issues:
    // a) KWorkSpace's DBus alls are all syncronous
    // b) the destrution of the menu that this action is in is delayed
    //
    // (a) leads to the menu hanging out where everyone can see it because
    // the even loop doesn't get returned to allowing it to close.
    //
    // (b) leads to a 0ms timer not working since a 0ms timer just appends to
    // the event queue, and then the menu closing event gets appended to that.
    //
    // ergo a timer with small timeout
    QTimer::singleShot(10, this, &ContextMenu::logout);
}

void ContextMenu::logout()
{
    if (!KAuthorized::authorizeAction(QStringLiteral("logout"))) {
        return;
    }

    KWorkSpace::requestShutDown();
}

QWidget* ContextMenu::createConfigurationInterface(QWidget* parent)
{
    QWidget *widget = new QWidget(parent);
    QVBoxLayout *lay = new QVBoxLayout();
    widget->setLayout(lay);
    widget->setWindowTitle(i18nc("plasma_containmentactions_contextmenu", "Configure Contextual Menu Plugin"));
    m_buttons = new QButtonGroup(widget);
    m_buttons->setExclusive(false);

    foreach (const QString &name, m_actionOrder) {
        QCheckBox *item = 0;

        if (name == QLatin1String("_context")) {
            item = new QCheckBox(widget);
            //FIXME better text
            item->setText(i18nc("plasma_containmentactions_contextmenu", "[Other Actions]"));
        } else if (name == QLatin1String("_wallpaper")) {
            item = new QCheckBox(widget);
            item->setText(i18nc("plasma_containmentactions_contextmenu", "Wallpaper Actions"));
            item->setIcon(QIcon::fromTheme(QStringLiteral("user-desktop")));
        } else if (name == QLatin1String("_sep1") || name ==QLatin1String("_sep2") || name == QLatin1String("_sep3")) {
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

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(contextmenu, ContextMenu, "plasma-containmentactions-contextmenu.json")

#include "menu.moc"
