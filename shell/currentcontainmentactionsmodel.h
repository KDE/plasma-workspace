/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QStandardItemModel>

#include <kconfig.h>
#include <kconfiggroup.h>

namespace Plasma
{
class Containment;
class ContainmentActions;
}

class QQuickItem;

// This model load the data about available containment actions plugins, such as context menus that can be loaded on mouse click
// TODO: out of the library?
class CurrentContainmentActionsModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Roles {
        ActionRole = Qt::UserRole + 1,
        PluginNameRole,
        HasConfigurationInterfaceRole,
    };

    explicit CurrentContainmentActionsModel(Plasma::Containment *containment, QObject *parent = nullptr);
    ~CurrentContainmentActionsModel() override;

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool isTriggerUsed(const QString &trigger);
    Q_INVOKABLE QString mouseEventString(int mouseButtons, int modifiers);
    Q_INVOKABLE QString wheelEventString(QObject *quickWheelEvent);
    Q_INVOKABLE bool append(const QString &action, const QString &plugin);
    Q_INVOKABLE void update(int row, const QString &action, const QString &plugin);
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void showConfiguration(int row, QQuickItem *ctx = nullptr);
    Q_INVOKABLE QVariant aboutMetaData(int row) const;
    Q_INVOKABLE void save();

Q_SIGNALS:
    void configurationChanged();

private:
    Plasma::Containment *m_containment;
    QHash<QString, Plasma::ContainmentActions *> m_plugins;
    KConfigGroup m_baseCfg;
    KConfigGroup m_tempConfig;
    KConfig m_tempConfigParent;
    QStringList m_removedTriggers;
};
