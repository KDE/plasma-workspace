/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QBindable>
#include <QConcatenateTablesProxyModel>
#include <QSize>

#include "model/imageroles.h"

class ImageProxyModel;

class SlideModel : public QConcatenateTablesProxyModel, public ImageRoles
{
    Q_OBJECT

public:
    explicit SlideModel(const QBindable<QSize> &bindableTargetSize, const QBindable<bool> &bindableUsedInConfig, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    int indexOf(const QString &packagePath) const;
    void openContainingFolder(int rowIndex) const;

    /**
     * @return added directories
     */
    QStringList addDirs(const QStringList &dirs);
    QString removeDir(const QString &selected);
    void setSlidePaths(const QStringList &slidePaths);

    void setUncheckedSlides(const QStringList &uncheckedSlides);

    QBindable<bool> loading() const;

Q_SIGNALS:
    void done();

private Q_SLOTS:
    void slotSourceModelLoadingChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(SlideModel, QSize, m_targetSize)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SlideModel, bool, m_usedInConfig, true)

    QHash<QString, ImageProxyModel *> m_models;
    int m_loaded = 0;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SlideModel, bool, m_loading, false)

    QHash<QString, bool> m_checkedTable;

    friend class SlideModelTest;
};
