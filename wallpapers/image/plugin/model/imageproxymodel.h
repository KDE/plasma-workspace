/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef IMAGEPROXYMODEL_H
#define IMAGEPROXYMODEL_H

#include <QConcatenateTablesProxyModel>
#include <QSize>

#include <KDirWatch>

#include "imageroles.h"

class AbstractImageListModel;
class ImageListModel;
class PackageListModel;
class XmlImageListModel;
class VideoListModel;

static constexpr int s_modelNum = 3;

/**
 * A proxy model that aggregates data from ImageListModel and PackageListModel.
 */
class ImageProxyModel : public QConcatenateTablesProxyModel, public ImageRoles
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /**
     * The number of video backgrounds
     */
    Q_PROPERTY(int videoCount READ videoCount NOTIFY videoCountChanged)

public:
    explicit ImageProxyModel(const QStringList &customPaths, const QSize &targetSize, QObject *parent);

    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    int videoCount() const;
    Q_INVOKABLE int indexOf(const QString &packagePath) const;

    bool loading() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QStringList addBackground(const QString &_path);
    void removeBackground(const QString &packagePath);

    Q_INVOKABLE void commitAddition();
    Q_INVOKABLE void commitDeletion();

    Q_INVOKABLE void openContainingFolder(int row) const;

Q_SIGNALS:
    void countChanged();
    void videoCountChanged();
    void loadingChanged();
    void targetSizeChanged(const QSize &size);

private Q_SLOTS:
    void slotHandleLoaded(AbstractImageListModel *model);

    /**
     * Slots to handle item changes in source models.
     */
    void slotSourceModelAboutToBeReset();
    void slotSourceModelReset();

    /**
     * Slots to handle file change signals from KDirWatch
     */
    void slotDirWatchCreated(const QString &path);
    void slotDirWatchDeleted(const QString &path);

private:
    ImageListModel *m_imageModel;
    PackageListModel *m_packageModel;
    XmlImageListModel *m_xmlModel = nullptr;
    VideoListModel *m_videoModel = nullptr;

    KDirWatch m_dirWatch;

    int m_loaded = 0;

    QStringList m_pendingAddition;

    friend class ImageProxyModelTest;
};

#endif // IMAGEPROXYMODEL_H
