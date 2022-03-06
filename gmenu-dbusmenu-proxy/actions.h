/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "gdbusmenutypes_p.h"

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

Q_SIGNALS:
    void loaded();
    void failedToLoad(); // expose error?
    void actionsChanged(const QStringList &dirtyActions);

private Q_SLOTS:
    void onActionsChanged(const QStringList &removed, const StringBoolMap &enabledChanges, const QVariantMap &stateChanges, const GMenuActionMap &added);

private:
    GMenuActionMap m_actions;

    QString m_serviceName;
    QString m_objectPath;
};
