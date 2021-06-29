/*
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct IconSizeCategoryModelData {
    QString configKey;
    QString display;
    QString configSection;
    int kIconloaderGroup;
};

Q_DECLARE_TYPEINFO(IconSizeCategoryModelData, Q_MOVABLE_TYPE);

class IconSizeCategoryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    IconSizeCategoryModel(QObject *parent);
    ~IconSizeCategoryModel() override;

    enum Roles {
        ConfigKeyRole = Qt::UserRole + 1,
        ConfigSectionRole,
        KIconLoaderGroupRole,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void load();

Q_SIGNALS:
    void categorySelectedIndexChanged();

private:
    QVector<IconSizeCategoryModelData> m_data;
};
