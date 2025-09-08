/*
    glibclocaleconstructor.cpp
    SPDX-FileCopyrightText: 2025 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "glibclocaleconstructor.h"
#include <KLocalizedString>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>

GlibcLocaleConstructor::GlibcLocaleConstructor()
    : m_localectl()
    , m_map({{QStringLiteral("C"), QStringLiteral("C")}})
    , m_enabled()
{
    // fedora pre generate locales, fetch available locales from localectl. /usr/share/i18n/locales is empty in fedora
    QDir glibcLocaleDir(localeFileDirPath());
    if (glibcLocaleDir.isEmpty()) {
        auto localectlPath = QStandardPaths::findExecutable(QStringLiteral("localectl"));
        if (!localectlPath.isEmpty()) {
            m_localectl = new QProcess(this);
            m_localectl->setProgram(localectlPath);
            m_localectl->setArguments({QStringLiteral("list-locales"), QStringLiteral("--no-pager")});
            connect(m_localectl, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
                m_enabled = true; // set to true even if failed. otherwise our failed notification is also grey out
                if (exitCode != 0 || status != QProcess::NormalExit) {
                    Q_EMIT encountedError(failedFindLocalesMessage());
                } else {
                    this->constructGlibcLocaleMap();
                }
                Q_EMIT enabledChanged();
            });
            m_localectl->start();
        } else {
            m_enabled = true;
        }
    } else {
        constructGlibcLocaleMap();
        m_enabled = true;
    }
}

GlibcLocaleConstructor *GlibcLocaleConstructor::instance()
{
    static GlibcLocaleConstructor singleton;
    return &singleton;
}

std::optional<QString> GlibcLocaleConstructor::toGlibcLocale(const QString &lang)
{
    if (m_map.contains(lang)) {
        return m_map[lang];
    }
    return std::nullopt;
}

void GlibcLocaleConstructor::constructGlibcLocaleMap()
{
    QDir glibcLocaleDir(localeFileDirPath());
    auto availableLocales = glibcLocaleDir.entryList(QDir::Files);
    // not glibc system or corrupted system
    if (availableLocales.isEmpty()) {
        if (m_localectl) {
            availableLocales = QString::fromLocal8Bit(m_localectl->readAllStandardOutput()).split(u'\n');
        }
        if (availableLocales.isEmpty()) {
            Q_EMIT encountedError(failedFindLocalesMessage());
            return;
        }
    }

    // map base locale code to actual glibc locale filename: "en" => ["en_US", "en_GB"]
    std::unordered_map<QString, std::vector<QString>> basem_map(availableLocales.size());
    m_availableLocales.reserve(availableLocales.size());
    for (const auto &glibcLocale : availableLocales) {
        // we want only absolute base locale code, for sr@ijekavian and en_US, we get sr and en
        auto baseLocale = glibcLocale.split(u'_')[0].split(u'@')[0];
        // clear glibcLocale from .UTF-8 and other similar items since it can break comparison
        auto glibcLocaleName = glibcLocale.split(u'.')[0];
        m_availableLocales.insert(glibcLocaleName);
        if (basem_map.contains(baseLocale)) {
            basem_map[baseLocale].push_back(glibcLocaleName);
        } else {
            basem_map.insert({baseLocale, {glibcLocaleName}});
        }
    }

    const auto addToMap = [this](const QString &plasmaLocale, const QString &glibcLocale) {
        // We map the locale name and the plasma name to a valid locale. This gives us flexibility for resolution
        // as we can resolve pt=>pt_PT but also pt_PT=>pt_PT.
        // https://mail.kde.org/pipermail/kde-i18n-doc/2023-January/001340.html
        // https://bugs.kde.org/show_bug.cgi?id=478120
        m_map.insert({glibcLocale, toUTF8Locale(glibcLocale)});
        m_map.insert({plasmaLocale, toUTF8Locale(glibcLocale)});
    };

    auto plasmaLocales = KLocalizedString::availableDomainTranslations(QByteArrayLiteral("plasmashell")).values();
    for (const auto &plasmaLocale : plasmaLocales) {
        auto baseLocale = plasmaLocale.split(u'_')[0].split(u'@')[0];
        if (basem_map.contains(baseLocale)) {
            const auto &prefixedLocales = basem_map[baseLocale];

            // if we have one to one match, use that. Eg. en_US to en_US
            auto fullMatch = std::ranges::find(prefixedLocales, plasmaLocale);
            if (fullMatch != prefixedLocales.end()) {
                addToMap(plasmaLocale, *fullMatch);
                continue;
            }

            // language name with same country code has higher priority, eg. es_ES > es_PA, de_DE > de_DE@euro
            const QString mainLocale = plasmaLocale + u'_' + plasmaLocale.toUpper();
            fullMatch = std::ranges::find(prefixedLocales, mainLocale);
            if (fullMatch != prefixedLocales.end()) {
                addToMap(plasmaLocale, *fullMatch);
                continue;
            }

            // we try to match the locale with least characters diff (compare language code with country code, "sv" with "SE".lower() for "sv_SE"),
            // so ca@valencia matches with ca_ES@valencia
            // bad case: ca matches with ca_AD but not ca_ES
            int closestMatchIndex = 0;
            float minDiffPercentage = 1.0;
            std::array<int, 255> frequencyMap = {0};
            for (QChar c : plasmaLocale) {
                // to lower so "sv_SE" has higher priority than "sv_FI" for language "sv"
                frequencyMap[int(c.toLower().toLatin1())]++;
            }

            int i = 0;
            for (const auto &glibcLocale : prefixedLocales) {
                auto duplicated = frequencyMap;
                auto skipBase = baseLocale.size() + 1; // we skip "sv_" part of "sv_SE", eg. compare "SE" part with "sv"
                for (QChar c : glibcLocale) {
                    if (skipBase--) {
                        continue;
                    }
                    duplicated[int(c.toLower().toLatin1())]--;
                }
                int diffChars = std::reduce(duplicated.begin(), duplicated.end(), 0, [](int sum, int diff) {
                    return sum + std::abs(diff);
                });
                float diffPercentage = float(diffChars) / glibcLocale.size();
                if (diffPercentage < minDiffPercentage) {
                    minDiffPercentage = diffPercentage;
                    closestMatchIndex = i;
                }
                i++;
            }
            addToMap(plasmaLocale, prefixedLocales[closestMatchIndex]);
        }
    }
}

QString GlibcLocaleConstructor::localeFileDirPath()
{
    return QStringLiteral("/usr/share/i18n/locales");
}

QString GlibcLocaleConstructor::failedFindLocalesMessage()
{
    return xi18nc("@info this will be shown as an error message",
                  "Could not find the system's available locales using the <command>localectl</command> tool. Please file a bug report about this at "
                  "<link>https://bugs.kde.org</link>");
}

QString GlibcLocaleConstructor::toUTF8Locale(const QString &locale)
{
    if (locale.contains(QLatin1String("UTF-8"))) {
        return locale;
    }

    if (locale.contains(QLatin1Char('@'))) {
        // uz_UZ@cyrillic to uz_UZ.UTF-8@cyrillic
        auto localeDup = locale;
        localeDup.replace(QLatin1Char('@'), QLatin1String(".UTF-8@"));
        return localeDup;
    }

    return locale + QLatin1String(".UTF-8");
}

bool GlibcLocaleConstructor::hasGlibcLocale(const QString &locale) const
{
    return m_availableLocales.contains(locale);
}

bool GlibcLocaleConstructor::enabled() const
{
    return m_enabled;
}
