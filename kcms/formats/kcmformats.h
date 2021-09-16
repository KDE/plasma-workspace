/*
    kcmformats.cpp
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

class FormatsSettings;
class KCMFormats : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(FormatsSettings *settings READ settings CONSTANT)
    Q_PROPERTY(QString numberExample READ numberExample NOTIFY numericExampleChanged)
    Q_PROPERTY(QString timeExample READ timeExample NOTIFY timeExampleChanged)
    Q_PROPERTY(QString currencyExample READ currencyExample NOTIFY monetaryExampleChanged)
    Q_PROPERTY(QString measurementExample READ measurementExample NOTIFY measurementExampleChanged)
    Q_PROPERTY(QString collateExample READ collateExample NOTIFY collateExampleChanged)
public:
    explicit KCMFormats(QObject *parent = nullptr, const QVariantList &list = QVariantList());
    virtual ~KCMFormats() override = default;

    QString numberExample() const;
    QString timeExample() const;
    QString currencyExample() const;
    QString measurementExample() const;
    QString collateExample() const;
    FormatsSettings *settings() const;
    Q_INVOKABLE QQuickItem *getSubPage(int index) const; // proxy from KQuickAddons to Qml
public Q_SLOTS:
    void handleLangChange();
Q_SIGNALS:
    void numericExampleChanged();
    void timeExampleChanged();
    void collateExampleChanged();
    void monetaryExampleChanged();
    void measurementExampleChanged();

private:
    QHash<QString, QString> m_cachedFlags;

    FormatsSettings *m_settings = nullptr;
};
