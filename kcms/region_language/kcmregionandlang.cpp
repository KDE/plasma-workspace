/*
    kcmregionandlang.cpp
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>
    SPDX-FileCopyrightText: 2023 Serenity Cybersecurity, LLC <license@futurecrew.ru>
                                 Author: Gleb Popov <arrowd@FreeBSD.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcmregionandlang.h"

#include <unistd.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDir>
#include <QQuickImageProvider>

#include <KCountryFlagEmojiIconEngine>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include "binarydialectmodel.h"
#include "languagelistmodel.h"
#include "localegenerator.h"
#include "localegeneratorbase.h"
#include "localelistmodel.h"
#include "optionsmodel.h"
#include "regionandlangsettings.h"

using namespace Qt::StringLiterals;
using namespace KCM_RegionAndLang;

K_PLUGIN_CLASS_WITH_JSON(KCMRegionAndLang, "kcm_regionandlang.json")

class FlagImageProvider : public QQuickImageProvider
{
public:
    FlagImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *, const QSize &requestedSize) override
    {
        CacheContext cacheContext(id, requestedSize);
        if (const auto it = m_pixmapCache.find(cacheContext); it != m_pixmapCache.end()) {
            return it.value();
        }
        return m_pixmapCache[cacheContext] = QIcon(new KCountryFlagEmojiIconEngine(id)).pixmap(requestedSize);
    }

private:
    using CacheContext = std::pair<QString, QSize>;
    QHash<CacheContext, QPixmap> m_pixmapCache;
};

KCMRegionAndLang::KCMRegionAndLang(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
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

    qmlRegisterAnonymousType<RegionAndLangSettings>("kcmregionandlang", 1);
    qmlRegisterAnonymousType<OptionsModel>("kcmregionandlang", 1);
    qmlRegisterAnonymousType<SelectedLanguageModel>("kcmregionandlang", 1);
    qmlRegisterType<LocaleListModel>("kcmregionandlang", 1, 0, "LocaleListModel");
    qmlRegisterType<LanguageListModel>("kcmregionandlang", 1, 0, "LanguageListModel");
    qmlRegisterType<BinaryDialectModel>("kcmregionandlang", 1, 0, "BinaryDialectModel");
    qRegisterMetaType<KCM_RegionAndLang::SettingType>();
    qmlRegisterUncreatableMetaObject(KCM_RegionAndLang::staticMetaObject,
                                     "kcmregionandlang",
                                     1,
                                     0,
                                     "SettingType",
                                     QStringLiteral("Error: SettingType is an enum"));

#if GLIBC_LOCALE_GENERATED
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
#else
    m_enabled = true;
#endif

    m_loadedBinaryDialect = m_optionsModel->binaryDialect();

    connect(m_optionsModel, &OptionsModel::binaryDialectChanged, this, [this]() {
        setNeedsSave(m_settings->isSaveNeeded() || m_loadedBinaryDialect != m_optionsModel->binaryDialect());
        setRepresentsDefaults(m_settings->isDefaults() && m_optionsModel->binaryDialect() == KFormat::BinaryUnitDialect::IECBinaryDialect);
    });

    auto setLocaleCall = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.locale1"),
                                                        QStringLiteral("/org/freedesktop/locale1"),
                                                        QStringLiteral("org.freedesktop.DBus.Introspectable"),
                                                        QStringLiteral("Introspect"));
    QDBusPendingCall async = QDBusConnection::systemBus().asyncCall(setLocaleCall);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, &KCMRegionAndLang::handleLocaledAvailable);
}

QString KCMRegionAndLang::failedFindLocalesMessage()
{
    return xi18nc("@info this will be shown as an error message",
                  "Could not find the system's available locales using the <command>localectl</command> tool. Please file a bug report about this at "
                  "<link>https://bugs.kde.org</link>");
}

QString KCMRegionAndLang::localeFileDirPath()
{
    return QStringLiteral("/usr/share/i18n/locales");
}

void KCMRegionAndLang::save()
{
    if (settings()->isSaveNeeded()) {
        Q_EMIT saveClicked();
    }
}

void KCMRegionAndLang::applyToLocal()
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

#ifdef GLIBC_LOCALE
        if (!settings()->language().isEmpty()) {
            QStringList languages = settings()->language().split(QLatin1Char(':'));
            for (const QString &lang : languages) {
                auto glibcLocale = toGlibcLocale(lang);
                if (glibcLocale.has_value()) {
                    locales.append(glibcLocale.value());
                }
            }
        }
#endif

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
            // after clicking "defaults" so all the settings are default
            saveToConfigFile();
        }

    KSharedConfigPtr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup localeGroup(globalConfig, QStringLiteral("Locale"));
    if (m_optionsModel->binaryDialect() == KFormat::IECBinaryDialect) {
        // no need to store default value
        if (localeGroup.hasKey("BinaryUnitDialect")) {
            localeGroup.deleteEntry("BinaryUnitDialect");
            globalConfig->sync();
        }
    } else {
        m_loadedBinaryDialect = m_optionsModel->binaryDialect();
        localeGroup.writeEntry("BinaryUnitDialect", m_loadedBinaryDialect);
        globalConfig->sync();

        Q_EMIT takeEffectNextTime();
    }
}

void KCMRegionAndLang::load()
{
    KQuickManagedConfigModule::load();
    engine()->addImageProvider(u"flags"_s, new FlagImageProvider);

    m_settings->load();
    m_optionsModel->load();

    Q_EMIT loadClicked();
}
void KCMRegionAndLang::defaults()
{
    KQuickManagedConfigModule::defaults();
    m_optionsModel->setBinaryDialect(KFormat::BinaryUnitDialect::IECBinaryDialect);
    Q_EMIT defaultsClicked();
}

void KCMRegionAndLang::applyToSystem()
{
    QStringList args;
    if (!settings()->isDefaultSetting(SettingType::Lang)) {
        args.append(QStringLiteral("LANG=%1").arg(settings()->lang()));
    }
    if (!settings()->isDefaultSetting(SettingType::Numeric)) {
        args.append(QStringLiteral("LC_NUMERIC=%1").arg(settings()->numeric()));
    }
    if (!settings()->isDefaultSetting(SettingType::Time)) {
        args.append(QStringLiteral("LC_TIME=%1").arg(settings()->time()));
    }
    if (!settings()->isDefaultSetting(SettingType::Measurement)) {
        args.append(QStringLiteral("LC_MEASUREMENT=%1").arg(settings()->measurement()));
    }
    if (!settings()->isDefaultSetting(SettingType::Currency)) {
        args.append(QStringLiteral("LC_MONETARY=%1").arg(settings()->monetary()));
    }
    if (!settings()->isDefaultSetting(SettingType::PaperSize)) {
        args.append(QStringLiteral("LC_PAPER=%1").arg(settings()->paperSize()));
    }
    if (!settings()->isDefaultSetting(SettingType::Address)) {
        args.append(QStringLiteral("LC_ADDRESS=%1").arg(settings()->address()));
    }
    if (!settings()->isDefaultSetting(SettingType::NameStyle)) {
        args.append(QStringLiteral("LC_NAME=%1").arg(settings()->nameStyle()));
    }
    if (!settings()->isDefaultSetting(SettingType::PhoneNumbers)) {
        args.append(QStringLiteral("LC_TELEPHONE=%1").arg(settings()->phoneNumbers()));
    }

    // SetLocale call will generate locales for us
    setSystemLocale(args);
}

void KCMRegionAndLang::saveCanceled()
{
    settingsChanged();
}

void KCMRegionAndLang::saveToConfigFile()
{
    KQuickManagedConfigModule::save();
}

void KCMRegionAndLang::handleLocaledAvailable(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString> reply = *call;
    if (reply.isError()) {
        qWarning("failed to introspect org.freedesktop.locale1, disable `applyToSystem`");
    } else {
        m_localedAvailable = true;
        Q_EMIT localedAvailableChanged();
    }
    call->deleteLater();
}

RegionAndLangSettings *KCMRegionAndLang::settings() const
{
    return m_settings;
}

OptionsModel *KCMRegionAndLang::optionsModel() const
{
    return m_optionsModel;
}

void KCMRegionAndLang::unset(SettingType setting) const
{
    if (setting == SettingType::BinaryDialect) {
        m_optionsModel->setBinaryDialect(KFormat::IECBinaryDialect);
        return;
    }

    const char *entry = nullptr;
    switch (setting) {
    case SettingType::Language:
        Q_ASSERT(false); // shouldn't happen
        return;
    case SettingType::Lang:
        entry = "LANG";
        settings()->setLang(settings()->defaultLangValue());
        break;
    case SettingType::Numeric:
        entry = "LC_NUMERIC";
        settings()->setNumeric(settings()->defaultNumericValue());
        break;
    case SettingType::Time:
        entry = "LC_TIME";
        settings()->setTime(settings()->defaultTimeValue());
        break;
    case SettingType::Measurement:
        entry = "LC_MEASUREMENT";
        settings()->setMeasurement(settings()->defaultMeasurementValue());
        break;
    case SettingType::Currency:
        entry = "LC_MONETARY";
        settings()->setMonetary(settings()->defaultMonetaryValue());
        break;
    case SettingType::PaperSize:
        entry = "LC_PAPER";
        settings()->setPaperSize(settings()->defaultPaperSizeValue());
        break;
    case SettingType::Address:
        entry = "LC_ADDRESS";
        settings()->setAddress(settings()->defaultAddressValue());
        break;
    case SettingType::NameStyle:
        entry = "LC_NAME";
        settings()->setNameStyle(settings()->defaultNameStyleValue());
        break;
    case SettingType::PhoneNumbers:
        entry = "LC_TELEPHONE";
        settings()->setPhoneNumbers(settings()->defaultPhoneNumbersValue());
        break;
    case SettingType::BinaryDialect:
        Q_UNREACHABLE();
        break;
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

bool KCMRegionAndLang::enabled() const
{
    return m_enabled;
}

bool KCMRegionAndLang::localedAvailable() const
{
    return m_localedAvailable;
}

#ifdef GLIBC_LOCALE
std::optional<QString> KCMRegionAndLang::toGlibcLocale(const QString &lang)
{
    static std::unordered_map<QString, QString> map = constructGlibcLocaleMap();

    if (map.contains(lang)) {
        return map[lang];
    }
    return std::nullopt;
}
#endif

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

#ifdef GLIBC_LOCALE
std::unordered_map<QString, QString> KCMRegionAndLang::constructGlibcLocaleMap()
{
    std::unordered_map<QString, QString> localeMap;

    QDir glibcLocaleDir(localeFileDirPath());
    auto availableLocales = glibcLocaleDir.entryList(QDir::Files);
    // not glibc system or corrupted system
    if (availableLocales.isEmpty()) {
        if (m_localectl) {
            availableLocales = QString::fromLocal8Bit(m_localectl->readAllStandardOutput()).split(u'\n');
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
        auto baseLocale = glibcLocale.split(u'_')[0].split(u'@')[0];
        if (baseLocaleMap.contains(baseLocale)) {
            baseLocaleMap[baseLocale].push_back(glibcLocale);
        } else {
            baseLocaleMap.insert({baseLocale, {glibcLocale}});
        }
    }

    const auto addToMap = [&localeMap](const QString &plasmaLocale, const QString &glibcLocale) {
        // We map the locale name and the plasma name to a valid locale. This gives us flexibility for resolution
        // as we can resolve pt=>pt_PT but also pt_PT=>pt_PT.
        // https://mail.kde.org/pipermail/kde-i18n-doc/2023-January/001340.html
        // https://bugs.kde.org/show_bug.cgi?id=478120
        localeMap.insert({glibcLocale, toUTF8Locale(glibcLocale)});
        localeMap.insert({plasmaLocale, toUTF8Locale(glibcLocale)});
    };

    auto plasmaLocales = KLocalizedString::availableDomainTranslations(QByteArrayLiteral("plasmashell")).values();
    for (const auto &plasmaLocale : plasmaLocales) {
        auto baseLocale = plasmaLocale.split(u'_')[0].split(u'@')[0];
        if (baseLocaleMap.contains(baseLocale)) {
            const auto &prefixedLocales = baseLocaleMap[baseLocale];

            // if we have one to one match, use that. Eg. en_US to en_US
            auto fullMatch = std::find(prefixedLocales.begin(), prefixedLocales.end(), plasmaLocale);
            if (fullMatch != prefixedLocales.end()) {
                addToMap(plasmaLocale, *fullMatch);
                continue;
            }

            // language name with same country code has higher priority, eg. es_ES > es_PA, de_DE > de_DE@euro
            auto mainLocale = plasmaLocale + u'_' + plasmaLocale.toUpper();
            fullMatch = std::find(prefixedLocales.begin(), prefixedLocales.end(), mainLocale);
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
    return localeMap;
}
#endif

bool KCMRegionAndLang::isDefaults() const
{
    return m_settings->isDefaults() && m_optionsModel->binaryDialect() == KFormat::BinaryUnitDialect::IECBinaryDialect;
}

void KCMRegionAndLang::getSystemLocale(const std::function<void(const QStringList &)> &lambda)
{
    // try to get current system locale
    auto getSystemLocaleCall = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.locale1"),
                                                              QStringLiteral("/org/freedesktop/locale1"),
                                                              QStringLiteral("org.freedesktop.DBus.Properties"),
                                                              QStringLiteral("Get"));
    getSystemLocaleCall.setArguments({QStringLiteral("org.freedesktop.locale1"), QStringLiteral("Locale")});
    auto async = QDBusConnection::systemBus().asyncCall(getSystemLocaleCall);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    std::function<void(const QStringList &)> copied = lambda;
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, [this, copied](QDBusPendingCallWatcher *call) {
        // locale1 actually returns StringList, but Qt can't parse the reply if I use QStringList here
        QDBusPendingReply<QVariant> reply = *call;
        if (reply.isError()) {
            qWarning() << "failed to get Locale from org.freedesktop.locale1: " << reply.error().message();
        } else {
            auto variant = reply.value();
            if (variant.type() != QVariant::StringList) {
                qWarning() << "failed to get Locale from org.freedesktop.locale1: reply type isn't StringList";
            } else {
                copied(variant.toStringList());
            }
        }
        call->deleteLater();
    });
}

void KCMRegionAndLang::setSystemLocale(const QStringList &locale)
{
    auto setLocaleCall = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.locale1"),
                                                        QStringLiteral("/org/freedesktop/locale1"),
                                                        QStringLiteral("org.freedesktop.locale1"),
                                                        QStringLiteral("SetLocale"));
    setLocaleCall.setArguments({locale, true});
    auto async = QDBusConnection::systemBus().asyncCall(setLocaleCall);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, [](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<void> reply = *call;
        if (reply.isError()) {
            qWarning() << "failed to set Locale via org.freedesktop.locale1: " << reply.error().message();
        }
        call->deleteLater();
    });
}

QQuickItem *KCMRegionAndLang::currentPage()
{
    // because subPage doesn't take mainUi into consideration,
    // but currentIndex does, substract one
    int _currentIndex = currentIndex() - 1;
    if (_currentIndex < 0) {
        return mainUi();
    } else {
        return subPage(currentIndex() - 1);
    }
}

#include "kcmregionandlang.moc"
#include "moc_kcmregionandlang.cpp"
