/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QHash>
#include <QLocale>
#include <QObject>

#include <unicode/tznames.h>

class TimezonesI18n : public QObject
{
    Q_OBJECT

public:
    explicit TimezonesI18n(QObject *parent = nullptr);
    Q_INVOKABLE QString i18nContinents(const QString &continent);
    Q_INVOKABLE QString i18nCountry(QLocale::Country country);
    Q_INVOKABLE QString i18nCity(const QString &timezoneId);

private:
    void init();

    QHash<QString, QString> m_i18nContinents;
    QScopedPointer<icu::TimeZoneNames> m_tzNames;
    bool m_isInitialized;
};
