/*
    optionsmodel.h
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>

#include "kformat.h"
#include "regionandlangsettings.h"

class RegionAndLangSettings;
class KCMRegionAndLang;

class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int binaryDialect READ binaryDialect WRITE setBinaryDialect NOTIFY binaryDialectChanged)

public:
    enum Roles {
        Name = Qt::DisplayRole,
        Subtitle,
        Example,
        Page,
    };
    explicit OptionsModel(KCMRegionAndLang *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    KFormat::BinaryUnitDialect binaryDialect();
    void setBinaryDialect(QVariant d);

public:
    void load();

Q_SIGNALS:
    void binaryDialectChanged();

public Q_SLOTS:
    void handleLangChange();

private:
    QString implicitFormatExampleMsg() const;
    QString getNativeName(const QString &locale) const;
    void updateBinaryDialectExample();

    QString m_numberExample;
    QString m_timeExample;
    QString m_currencyExample;
    QString m_measurementExample;
    QString m_paperSizeExample;
#ifdef LC_ADDRESS
    QString m_addressExample;
    QString m_nameStyleExample;
    QString m_phoneNumbersExample;
#endif

    std::vector<std::pair<QString, KCM_RegionAndLang::SettingType>> m_staticNames; // title, page

    RegionAndLangSettings *m_settings;

    QString m_binaryDialectExample;
    KFormat::BinaryUnitDialect m_binaryDialect;
};
