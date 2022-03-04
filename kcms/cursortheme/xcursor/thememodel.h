/*
    SPDX-FileCopyrightText: 2005-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#pragma once

#include <QAbstractTableModel>
#include <QStringList>

class QDir;
class CursorTheme;

/**
 * The CursorThemeModel class provides a model for all locally installed
 * Xcursor themes, and the KDE/Qt legacy bitmap theme.
 *
 * This class automatically scans the locations in the file system from
 * which Xcursor loads cursors, and creates an internal list of all
 * available cursor themes.
 *
 * The model provides this theme list to item views in the form of a list
 * of rows with two columns; the first column has the theme's descriptive
 * name and its sample cursor as its icon, and the second column contains
 * the theme's description.
 *
 * Additional Xcursor themes can be added to a model after it's been
 * created, by calling addTheme(), which takes QDir as a parameter,
 * with the themes location. The intention is for this function to be
 * called when a new Xcursor theme has been installed, after the model
 * was instantiated.
 *
 * The KDE legacy theme is a read-only entry, with the descriptive name
 * "KDE Classic", and the internal name "#kde_legacy#".
 *
 * Calling defaultIndex() will return the index of the theme Xcursor
 * will use if the user hasn't explicitly configured a cursor theme.
 */
class CursorThemeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CursorThemeModel(QObject *parent = nullptr);
    ~CursorThemeModel() override;
    QHash<int, QByteArray> roleNames() const override;
    inline int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    /// Returns the CursorTheme at @p index.
    const CursorTheme *theme(const QModelIndex &index);

    /// Returns the index for the CursorTheme with the internal name @p name,
    /// or an invalid index if no matching theme can be found.
    QModelIndex findIndex(const QString &name);

    /// Returns the index for the default theme.
    QModelIndex defaultIndex();

    /// Adds the theme in @p dir, and returns @a true if successful or @a false otherwise.
    bool addTheme(const QDir &dir);
    void removeTheme(const QModelIndex &index);

    /// Returns the list of base dirs Xcursor looks for themes in.
    const QStringList searchPaths();

    /// Refresh the list of themes by checking what's on disk.
    void refreshList();

private:
    bool handleDefault(const QDir &dir);
    void processThemeDir(const QDir &dir);
    void insertThemes();
    bool hasTheme(const QString &theme) const;
    bool isCursorTheme(const QString &theme, const int depth = 0);

private:
    QList<CursorTheme *> list;
    QStringList baseDirs;
    QString defaultName;
    QVector<CursorTheme *> pendingDeletions;
};

int CursorThemeModel::rowCount(const QModelIndex &) const
{
    return list.count();
}
