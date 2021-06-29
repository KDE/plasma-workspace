/*
    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    KDE Frameworks 5 port:
    SPDX-FileCopyrightText: 2013 Jonathan Riddell <jr@jriddell.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class IconsSettings;

struct IconsModelData {
    QString display;
    QString themeName;
    QString description;
    bool removable;
    bool pendingDeletion;
};
Q_DECLARE_TYPEINFO(IconsModelData, Q_MOVABLE_TYPE);

class IconsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    IconsModel(IconsSettings *iconsSettings, QObject *parent = nullptr);
    ~IconsModel() override;

    enum Roles {
        ThemeNameRole = Qt::UserRole + 1,
        DescriptionRole,
        RemovableRole,
        PendingDeletionRole,
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList pendingDeletions() const;
    void removeItemsPendingDeletion();

    void load();

Q_SIGNALS:
    void pendingDeletionsChanged();

private:
    QVector<IconsModelData> m_data;
    IconsSettings *m_settings;
};
