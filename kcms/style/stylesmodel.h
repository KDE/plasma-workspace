/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@pbroulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct StylesModelData {
    QString display;
    QString styleName;
    QString description;
    QString configPage;
};
Q_DECLARE_TYPEINFO(StylesModelData, Q_MOVABLE_TYPE);

class StylesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString selectedStyle READ selectedStyle WRITE setSelectedStyle NOTIFY selectedStyleChanged)
    Q_PROPERTY(int selectedStyleIndex READ selectedStyleIndex NOTIFY selectedStyleIndexChanged)

public:
    StylesModel(QObject *parent);
    ~StylesModel() override;

    enum Roles {
        StyleNameRole = Qt::UserRole + 1,
        DescriptionRole,
        ConfigurableRole,
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString selectedStyle() const;
    void setSelectedStyle(const QString &style);

    int indexOfStyle(const QString &style) const;
    int selectedStyleIndex() const;

    QString styleConfigPage(const QString &style) const;

    void load();

Q_SIGNALS:
    void selectedStyleChanged(const QString &style);
    void selectedStyleIndexChanged();

private:
    QString m_selectedStyle;

    QVector<StylesModelData> m_data;
};
