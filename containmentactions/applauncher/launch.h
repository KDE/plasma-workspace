/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QMenu>

#include <KServiceGroup>

#include <plasma/containmentactions.h>

#include "ui_config.h"

class QAction;

class AppLauncher : public Plasma::ContainmentActions
{
    Q_OBJECT
public:
    AppLauncher(QObject *parent, const QVariantList &args);
    ~AppLauncher() override;

    void init(const KConfigGroup &config);

    QList<QAction *> contextualActions() override;

    QWidget *createConfigurationInterface(QWidget *parent) override;
    void configurationAccepted() override;

    void restore(const KConfigGroup &config) override;
    void save(KConfigGroup &config) override;

protected:
    void makeMenu(QMenu *menu, const KServiceGroup::Ptr &group);

private:
    KServiceGroup::Ptr m_group;
    QList<QAction *> m_actions;

    Ui::Config m_ui;
    bool m_showAppsByName = false;
};
