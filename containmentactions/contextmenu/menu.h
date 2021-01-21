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

#ifndef CONTEXTMENU_HEADER
#define CONTEXTMENU_HEADER

#include <QButtonGroup>
#include <plasma/containmentactions.h>

class SessionManagement;

class ContextMenu : public Plasma::ContainmentActions
{
    Q_OBJECT
public:
    ContextMenu(QObject *parent, const QVariantList &args);
    ~ContextMenu() override;

    void restore(const KConfigGroup &) override;

    QList<QAction *> contextualActions() override;
    QAction *action(const QString &name);

    QWidget *createConfigurationInterface(QWidget *parent) override;
    void configurationAccepted() override;
    void save(KConfigGroup &config) override;

public Q_SLOTS:
    void runCommand();
    void startLogout();

private:
    QAction *m_runCommandAction;
    QAction *m_lockScreenAction;
    QAction *m_logoutAction;
    QAction *m_separator1;
    QAction *m_separator2;
    QAction *m_separator3;

    // action name and whether it is enabled or not
    QHash<QString, bool> m_actions;
    QStringList m_actionOrder;
    QButtonGroup *m_buttons;
    SessionManagement *m_session;
};

#endif
