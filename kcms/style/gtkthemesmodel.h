/*
    SPDX-FileCopyrightText: 2020 Mikhail Zolotukhin <zomial@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAbstractListModel>
#include <QMap>
#include <QStringList>

class QString;

struct GtkTheme {
    QString path;
    bool gtk2 = false;
    bool gtk3 = false;
    bool gtk4 = false;
};

class GtkThemesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString selectedTheme READ selectedTheme WRITE setSelectedTheme NOTIFY selectedThemeChanged)
    Q_PROPERTY(bool selectedThemeSupportsGtk4 READ selectedThemeSupportsGtk4 NOTIFY selectedThemeChanged)
    Q_PROPERTY(QList<int> selectedThemeMissingGtkVersions READ selectedThemeMissingGtkVersions NOTIFY selectedThemeChanged)

public:
    explicit GtkThemesModel(QObject *parent = nullptr);

    enum Roles {
        ThemeNameRole = Qt::UserRole + 1,
        ThemePathRole,
        GtkVersionsRole,
    };

    void load();

    void setThemesList(const QMap<QString, GtkTheme> &themes);

    void setSelectedTheme(const QString &themeName);
    QString selectedTheme();
    Q_SIGNAL void selectedThemeChanged(const QString &themeName);

    bool selectedThemeSupportsGtk4() const;
    QList<int> selectedThemeMissingGtkVersions() const;

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
    // mapping from theme name to theme, ordered by name
    QMap<QString, GtkTheme> m_themes;
};
