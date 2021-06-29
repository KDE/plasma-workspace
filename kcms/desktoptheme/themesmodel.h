/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAbstractListModel>
#include <QPalette>
#include <QString>
#include <QVector>

#include <memory>

struct ThemesModelData;

class ThemesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString selectedTheme READ selectedTheme WRITE setSelectedTheme NOTIFY selectedThemeChanged)
    Q_PROPERTY(int selectedThemeIndex READ selectedThemeIndex NOTIFY selectedThemeChanged)

public:
    ThemesModel(QObject *parent);
    ~ThemesModel() override;

    enum Roles {
        PluginNameRole = Qt::UserRole + 1,
        ThemeNameRole,
        DescriptionRole,
        FollowsSystemColorsRole,
        ColorTypeRole,
        IsLocalRole,
        PendingDeletionRole,
    };
    Q_ENUM(Roles)
    enum ColorType {
        LightTheme,
        DarkTheme,
        FollowsColorTheme,
    };
    Q_ENUM(ColorType)

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    QString selectedTheme() const;
    void setSelectedTheme(const QString &pluginName);

    int pluginIndex(const QString &pluginName) const;
    int selectedThemeIndex() const;

    QStringList pendingDeletions() const;
    void removeRow(int row);

    void load();

Q_SIGNALS:
    void selectedThemeChanged(const QString &pluginName);
    void selectedThemeIndexChanged();

    void pendingDeletionsChanged();

private:
    QString m_selectedTheme;
    // Can't use QVector because unique_ptr causes deletion of copy-ctor
    QVector<ThemesModelData> m_data;
};

struct ThemesModelData {
    QString display;
    QString pluginName;
    QString description;
    ThemesModel::ColorType type;
    bool isLocal;
    bool pendingDeletion;
};
