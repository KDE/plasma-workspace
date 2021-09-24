/*
    SPDX-FileCopyrightText: 2020 Mikhail Zolotukhin <zomial@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAbstractListModel>
#include <QMap>
#include <QStringList>

class QString;

class GtkThemesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString selectedTheme READ selectedTheme WRITE setSelectedTheme NOTIFY selectedThemeChanged)

public:
    GtkThemesModel(QObject *parent = nullptr);

    enum Roles {
        ThemeNameRole = Qt::UserRole + 1,
        ThemePathRole,
    };

    void load();

    void setThemesList(const QMap<QString, QString> &themes);
    QMap<QString, QString> themesList();

    void setSelectedTheme(const QString &themeName);
    QString selectedTheme();
    Q_SIGNAL void selectedThemeChanged(const QString &themeName);

    Q_INVOKABLE bool selectedThemeRemovable();
    Q_INVOKABLE void removeSelectedTheme();
    Q_INVOKABLE int findThemeIndex(const QString &themeName);
    Q_INVOKABLE void setSelectedThemeDirty();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Roles::ThemeNameRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void requestThemesListUpdate();

Q_SIGNALS:
    void themeRemoved();

private:
    QStringList possiblePathsToThemes();
    QString themePath(const QString &themeName);

    QString m_selectedTheme;
    // mapping from theme name to theme path, ordered by name
    QMap<QString, QString> m_themes;
};
