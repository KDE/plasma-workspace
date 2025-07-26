/*
    glibclocaleconstructor.h
    SPDX-FileCopyrightText: 2025 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <QProcess>
#include <optional>
#include <unordered_map>
#include <unordered_set>

class GlibcLocaleConstructor : public QObject
{
    Q_OBJECT
public:
    static GlibcLocaleConstructor *instance();

    std::optional<QString> toGlibcLocale(const QString &lang);

    bool enabled() const;
    bool hasGlibcLocale(const QString &locale) const;

    static QString toUTF8Locale(const QString &locale);

Q_SIGNALS:
    void encountedError(const QString &reason);
    void enabledChanged();

private:
    GlibcLocaleConstructor();
    static QString failedFindLocalesMessage();
    static QString localeFileDirPath();
    void constructGlibcLocaleMap();

    QProcess *m_localectl;
    std::unordered_map<QString, QString> m_map;
    std::unordered_set<QString> m_availableLocales;
    bool m_enabled;
};