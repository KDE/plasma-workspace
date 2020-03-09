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

#include "gdbusmenutypes_p.h"

class QStringList;

class Actions : public QObject
{
    Q_OBJECT

public:
    Actions(const QString &serviceName, const QString &objectPath, QObject *parent = nullptr);
    ~Actions() override;

    void load();

    bool get(const QString &name, GMenuAction &action) const;
    GMenuActionMap getAll() const;
    void trigger(const QString &name, const QVariant &target, uint timestamp = 0);

    bool isValid() const; // basically "has actions"

signals:
    void loaded();
    void failedToLoad(); // expose error?
    void actionsChanged(const QStringList &dirtyActions);

private slots:
    void onActionsChanged(const QStringList &removed,
                          const StringBoolMap &enabledChanges,
                          const QVariantMap &stateChanges,
                          const GMenuActionMap &added);

private:
    GMenuActionMap m_actions;

    QString m_serviceName;
    QString m_objectPath;

};
