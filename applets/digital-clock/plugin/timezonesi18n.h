/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef TIMEZONESI18N_H
#define TIMEZONESI18N_H

#include <QHash>
#include <QLocale>
#include <QObject>

class TimezonesI18n : public QObject
{
    Q_OBJECT

public:
    explicit TimezonesI18n(QObject *parent = nullptr);
    Q_INVOKABLE QString i18nCity(const QString &city);
    Q_INVOKABLE QString i18nContinents(const QString &continent);
    Q_INVOKABLE QString i18nCountry(QLocale::Country country);

private:
    void init();

    QHash<QString, QString> m_i18nCities;
    QHash<QString, QString> m_i18nContinents;
    QHash<QLocale::Country, QString> m_i18nCountries;
    bool m_isInitialized;
};

#endif // TIMEZONESI18N_H
