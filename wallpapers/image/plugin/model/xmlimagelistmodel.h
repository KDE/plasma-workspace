/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef XMLIMAGELISTMODEL_H
#define XMLIMAGELISTMODEL_H

#include "abstractimagelistmodel.h"

struct WallpaperItem;

/**
 * GNOME XML Background
 */
class XmlImageListModel : public AbstractImageListModel
{
    Q_OBJECT

public:
    explicit XmlImageListModel(const QSize &targetSize, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /**
     * @path URL string like image://gnome-wp-list/get?...
     */
    int indexOf(const QString &path) const override;

    void load(const QStringList &customPaths = {}) override;

public Q_SLOTS:
    /**
     * @path XML file path
     */
    QStringList addBackground(const QString &path) override;
    /**
     * @path URL string like image://gnome-wp-list/get?...
     * @return a list that contains the XML file and the image file
     */
    QStringList removeBackground(const QString &path) override;

private Q_SLOTS:
    void slotXmlFound(const QList<WallpaperItem> &packages);
    void slotXmlFinderGotPreview(const WallpaperItem &item, const QPixmap &preview);
    void slotXmlFinderFailed(const WallpaperItem &item);

private:
    void asyncGetXmlPreview(const WallpaperItem &item, const QPersistentModelIndex &index) const;

    QString getRealPath(const WallpaperItem &item) const;

    QList<WallpaperItem> m_data;

    friend class XmlImageListModelTest;
};

#endif // XMLIMAGELISTMODEL_H
