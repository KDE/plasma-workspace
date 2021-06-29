/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
