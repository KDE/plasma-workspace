/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbusmenumodel.h"

#include <QIcon>
#include <QKeySequence>
#include <QTimer>

class DBusMenuModelItem : public QObject
{
    Q_OBJECT

public:
    explicit DBusMenuModelItem(int id, const QVariantMap &data, QObject *parent = nullptr);
    ~DBusMenuModelItem() override;

    QList<int> update(const QVariantMap &data);
    QList<int> reset(const QStringList &data);

    int id() const;
    QString label() const;
    QIcon icon() const;
    bool isEnabled() const;
    bool isVisible() const;
    bool isChecked() const;
    bool isSubmenu() const;
    bool isSeparator() const;
    QString toggleType() const;
    QKeySequence shortcut() const;

    DBusMenuModelItem *parentItem() const;
    QList<DBusMenuModelItem *> childItems() const;
    int childCount() const;
    void insertChild(DBusMenuModelItem *item, int index);
    void removeChild(DBusMenuModelItem *item);
    void moveChild(int sourceIndex, int targetIndex);

private:
    QString m_label;
    QIcon m_icon;
    std::optional<size_t> m_iconHash = std::nullopt;
    QString m_toggleType;
    QKeySequence m_shortcut;
    int m_id;
    bool m_enabled = true;
    bool m_checked = false;
    bool m_visible = true;
    bool m_submenu = false;
    bool m_separator = false;
    DBusMenuModelItem *m_parentItem = nullptr;
    QList<DBusMenuModelItem *> m_childItems;
};

class DBusMenuModelPrivate
{
public:
    QSet<int> dirtyItems;
    QTimer dirtyTimer;

    std::unique_ptr<DBusMenuInterface> interface;
    std::unique_ptr<DBusMenuModelItem> rootItem;
    std::unordered_map<int, DBusMenuModelItem *> items;

    int prefetchSize = 1;
};
