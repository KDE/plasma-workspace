/*
    kcmregionandlang.cpp
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "config-workspace.h"

#include "kcmregionandlang.h"

#include <unistd.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include "languagelistmodel.h"
#include "localegenerator.h"
#include "localegeneratorbase.h"
#include "localelistmodel.h"
#include "optionsmodel.h"
#include "regionandlangsettings.h"

using namespace KCM_RegionAndLang;

K_PLUGIN_CLASS_WITH_JSON(KCMRegionAndLang, "kcm_regionandlang.json")

KCMRegionAndLang::KCMRegionAndLang(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_settings(new RegionAndLangSettings(this))
    , m_optionsModel(new OptionsModel(this))
    , m_generator(LocaleGenerator::getGenerator())
{
    connect(m_generator, &LocaleGeneratorBase::userHasToGenerateManually, this, &KCMRegionAndLang::userHasToGenerateManually);
    connect(m_generator, &LocaleGeneratorBase::success, this, &KCMRegionAndLang::generateFinished);
    connect(m_generator, &LocaleGeneratorBase::needsFont, this, &KCMRegionAndLang::requireInstallFont);
    connect(m_generator, &LocaleGeneratorBase::success, this, &KCMRegionAndLang::saveToConfigFile);
    connect(m_generator, &LocaleGeneratorBase::userHasToGenerateManually, this, &KCMRegionAndLang::saveToConfigFile);
    connect(m_generator, &LocaleGeneratorBase::needsFont, this, &KCMRegionAndLang::saveToConfigFile);

    // if we don't support auto locale generation for current system (BSD, musl etc.), userHasToGenerateManually regarded as success
    if (strcmp(m_generator->metaObject()->className(), "LocaleGeneratorBase") != 0) {
        connect(m_generator, &LocaleGeneratorBase::success, this, &KCMRegionAndLang::takeEffectNextTime);
    } else {
        connect(m_generator, &LocaleGeneratorBase::userHasToGenerateManually, this, &KCMRegionAndLang::takeEffectNextTime);
    }

    setQuickHelp(i18n("You can configure the formats used for time, dates, money and other numbers here."));

    qmlRegisterAnonymousType<RegionAndLangSettings>("kcmregionandlang", 1);
    qmlRegisterAnonymousType<OptionsModel>("kcmregionandlang", 1);
    qmlRegisterAnonymousType<SelectedLanguageModel>("kcmregionandlang", 1);
    qmlRegisterType<LocaleListModel>("kcmregionandlang", 1, 0, "LocaleListModel");
    qmlRegisterType<LanguageListModel>("kcmregionandlang", 1, 0, "LanguageListModel");
    qRegisterMetaType<KCM_RegionAndLang::SettingType>();
    qmlRegisterUncreatableMetaObject(KCM_RegionAndLang::staticMetaObject, "kcmregionandlang", 1, 0, "SettingType", "Error: SettingType is an enum");

    // fedora pre generate locales, fetch available locales from localectl. /usr/share/i18n/locales is empty in fedora
    QDir glibcLocaleDir(localeFileDirPath());
    if (glibcLocaleDir.isEmpty()) {
        auto localectlPath = QStandardPaths::findExecutable(QStringLiteral("localectl"));
        if (!localectlPath.isEmpty()) {
            m_localectl = new QProcess(this);
            m_localectl->setProgram(localectlPath);
            m_localectl->setArguments({QStringLiteral("list-locales")});
            connect(m_localectl, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
                m_enabled = true; // set to true even if failed. otherwise our failed notification is also grey out
                if (exitCode != 0 || status != QProcess::NormalExit) {
                    Q_EMIT encountedError(failedFindLocalesMessage());
                }
                Q_EMIT enabledChanged();
            });
            m_localectl->start();
        }
    } else {
        m_enabled = true;
    }
}

QString KCMRegionAndLang::failedFindLocalesMessage()
{
    return xi18nc("@info this will be shown as an error message",
                 "Could not find the system's available locales using the <command>localectl</command> tool. Please file a bug report about this at <link>https://bugs.kde.org</link>");
}

QString KCMRegionAndLang::localeFileDirPath()
{
    return QStringLiteral("/usr/share/i18n/locales");
}

void KCMRegionAndLang::save()
{
    // assemble full locales in use
    QStringList locales;
    if (!settings()->isDefaultSetting(SettingType::Lang)) {
        locales.append(settings()->lang());
    }
    if (!settings()->isDefaultSetting(SettingType::Numeric)) {
        locales.append(settings()->numeric());
    }
    if (!settings()->isDefaultSetting(SettingType::Time)) {
        locales.append(settings()->time());
    }
    if (!settings()->isDefaultSetting(SettingType::Measurement)) {
        locales.append(settings()->measurement());
    }
    if (!settings()->isDefaultSetting(SettingType::Currency)) {
        locales.append(settings()->monetary());
    }
    if (!settings()->isDefaultSetting(SettingType::PaperSize)) {
        locales.append(settings()->paperSize());
    }
    if (!settings()->isDefaultSetting(SettingType::Address)) {
        locales.append(settings()->address());
    }
    if (!settings()->isDefaultSetting(SettingType::NameStyle)) {
        locales.append(settings()->nameStyle());
    }
    if (!settings()->isDefaultSetting(SettingType::PhoneNumbers)) {
        locales.append(settings()->phoneNumbers());
    }
    if (!settings()->language().isEmpty()) {
        QStringList languages = settings()->language().split(QLatin1Char(':'));
        for (const QString &lang : languages) {
            auto glibcLocale = toGlibcLocale(lang);
            if (glibcLocale.has_value()) {
                locales.append(glibcLocale.value());
            }
        }
    }

    auto setLangCall = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.Accounts"),
                                                      QStringLiteral("/org/freedesktop/Accounts/User%1").arg(getuid()),
                                                      QStringLiteral("org.freedesktop.Accounts.User"),
                                                      QStringLiteral("SetLanguage"));
    setLangCall.setArguments({settings()->lang()});
    QDBusConnection::systemBus().asyncCall(setLangCall);

    if (!locales.isEmpty()) {
        Q_EMIT startGenerateLocale();
        m_generator->localesGenerate(locales);
    } else {
        // probably after clicking "defaults" so all the setting is default
        saveToConfigFile();
    }
    Q_EMIT saveClicked();
}

void KCMRegionAndLang::saveToConfigFile()
{
    KQuickAddons::ManagedConfigModule::save();
}

RegionAndLangSettings *KCMRegionAndLang::settings() const
{
    return m_settings;
}

OptionsModel *KCMRegionAndLang::optionsModel() const
{
    return m_optionsModel;
}

void KCMRegionAndLang::unset(SettingType setting)
{
    const char *entry = nullptr;
    if (setting == SettingType::Lang) {
        entry = "LANG";
        settings()->setLang(settings()->defaultLangValue());
    } else if (setting == SettingType::Numeric) {
        entry = "LC_NUMERIC";
        settings()->setNumeric(settings()->defaultNumericValue());
    } else if (setting == SettingType::Time) {
        entry = "LC_TIME";
        settings()->setTime(settings()->defaultTimeValue());
    } else if (setting == SettingType::Measurement) {
        entry = "LC_MEASUREMENT";
        settings()->setMeasurement(settings()->defaultMeasurementValue());
    } else if (setting == SettingType::Currency) {
        entry = "LC_MONETARY";
        settings()->setMonetary(settings()->defaultMonetaryValue());
    } else if (setting == SettingType::PaperSize) {
        entry = "LC_PAPER";
        settings()->setPaperSize(settings()->defaultPaperSizeValue());
    } else if (setting == SettingType::Address) {
        entry = "LC_ADDRESS";
        settings()->setAddress(settings()->defaultAddressValue());
    } else if (setting == SettingType::NameStyle) {
        entry = "LC_NAME";
        settings()->setNameStyle(settings()->defaultNameStyleValue());
    } else if (setting == SettingType::PhoneNumbers) {
        entry = "LC_TELEPHONE";
        settings()->setPhoneNumbers(settings()->defaultPhoneNumbersValue());
    }

    settings()->config()->group(QStringLiteral("Formats")).deleteEntry(entry);
}

void KCMRegionAndLang::reboot()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptReboot"));
    QDBusConnection::sessionBus().asyncCall(method);
}

bool KCMRegionAndLang::isGlibc()
{
#ifdef OS_UBUNTU
    return true;
#elif GLIBC_LOCALE
    return true;
#else
    return false;
#endif
}

bool KCMRegionAndLang::enabled() const
{
    return m_enabled;
}

std::optional<QString> KCMRegionAndLang::toGlibcLocale(const QString &lang)
{
    static std::unordered_map<QString, QString> map = constructGlibcLocaleMap();

    if (map.count(lang)) {
        return map[lang];
    }
    return std::nullopt;
}

QString KCMRegionAndLang::toUTF8Locale(const QString &locale)
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

std::unordered_map<QString, QString> KCMRegionAndLang::constructGlibcLocaleMap()
{
    std::unordered_map<QString, QString> localeMap;

    QDir glibcLocaleDir(localeFileDirPath());
    auto availableLocales = glibcLocaleDir.entryList(QDir::Files);
    // not glibc system or corrupted system
    if (availableLocales.isEmpty()) {
        if (m_localectl) {
            availableLocales = QString(m_localectl->readAllStandardOutput()).split('\n');
        }
        if (availableLocales.isEmpty()) {
            Q_EMIT encountedError(failedFindLocalesMessage());
            return localeMap;
        }
    }

    // map base locale code to actual glibc locale filename: "en" => ["en_US", "en_GB"]
    std::unordered_map<QString, std::vector<QString>> baseLocaleMap(availableLocales.size());
    for (const auto &glibcLocale : availableLocales) {
        // we want only absolute base locale code, for sr@ijekavian and en_US, we get sr and en
        auto baseLocale = glibcLocale.split('_')[0].split('@')[0];
        if (baseLocaleMap.count(baseLocale)) {
            baseLocaleMap[baseLocale].push_back(glibcLocale);
        } else {
            baseLocaleMap.insert({baseLocale, {glibcLocale}});
        }
    }

    auto plasmaLocales = KLocalizedString::availableDomainTranslations(QByteArrayLiteral("plasmashell")).values();
    for (const auto &plasmaLocale : plasmaLocales) {
        auto baseLocale = plasmaLocale.split('_')[0].split('@')[0];
        if (baseLocaleMap.count(baseLocale)) {
            const auto &prefixedLocales = baseLocaleMap[baseLocale];

            // if we have one to one match, use that. Eg. en_US to en_US
            auto fullMatch = std::find(prefixedLocales.begin(), prefixedLocales.end(), plasmaLocale);
            if (fullMatch != prefixedLocales.end()) {
                localeMap.insert({plasmaLocale, toUTF8Locale(*fullMatch)});
                continue;
            }

            // language name with same country code has higher priority, eg. es_ES > es_PA, de_DE > de_DE@euro
            auto mainLocale = plasmaLocale + "_" + plasmaLocale.toUpper();
            fullMatch = std::find(prefixedLocales.begin(), prefixedLocales.end(), mainLocale);
            if (fullMatch != prefixedLocales.end()) {
                localeMap.insert({plasmaLocale, toUTF8Locale(*fullMatch)});
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
                int skipBase = baseLocale.size() + 1; // we skip "sv_" part of "sv_SE", eg. compare "SE" part with "sv"
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
            localeMap.insert({plasmaLocale, toUTF8Locale(prefixedLocales[closestMatchIndex])});
        }
    }
    return localeMap;
}
#include "kcmregionandlang.moc"
#include "moc_kcmregionandlang.cpp"
