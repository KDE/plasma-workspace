/*
 * Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "gdbusmenutypes_p.h"
#include "../libdbusmenuqt/dbusmenutypes_p.h"

class Menu : public QObject
{
    Q_OBJECT

public:
    Menu(const QString &serviceName, const QString &objectPath, QObject *parent = nullptr);
    ~Menu() override;

    void init();
    void cleanup();

    void start(uint id);
    void stop(const QList<uint> &ids);

    bool hasMenu() const;
    bool hasSubscription(uint subscription) const;

    GMenuItem getSection(int id, bool *ok = nullptr) const;
    GMenuItem getSection(int subscription, int sectionId, bool *ok = nullptr) const;

    QVariantMap getItem(int id) const; // bool ok argument?
    QVariantMap getItem(int subscription, int sectionId, int id) const;

public slots:
    void actionsChanged(const QStringList &dirtyActions, const QString &prefix);

signals:
    void menuAppeared(); // emitted the first time a menu was successfully loaded
    void menuDisappeared();

    void subscribed(uint id);
    void failedToSubscribe(uint id);

    void itemsChanged(const QVector<uint> &itemIds);
    void menusChanged(const QVector<uint> &menuIds);

private slots:
    void onMenuChanged(const GMenuChangeList &changes);

private:
    void initMenu();

    void menuChanged(const GMenuChangeList &changes);

    // QSet?
    QList<uint> m_subscriptions; // keeps track of which menu trees we're subscribed to

    QHash<uint, GMenuItemList> m_menus;

    QString m_serviceName;
    QString m_objectPath;

};
