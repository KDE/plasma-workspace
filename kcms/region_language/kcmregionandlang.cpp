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
#include <QDir>
#include <QQuickImageProvider>

#include <KCountryFlagEmojiIconEngine>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include "binarydialectmodel.h"
#include "glibclocaleconstructor.h"
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
    connect(GlibcLocaleConstructor::instance(), &GlibcLocaleConstructor::encountedError, this, &KCMRegionAndLang::encountedError);
    connect(GlibcLocaleConstructor::instance(), &GlibcLocaleConstructor::enabledChanged, this, &KCMRegionAndLang::enabledChanged);

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

    m_loadedBinaryDialect = m_optionsModel->binaryDialect();

    connect(m_optionsModel, &OptionsModel::binaryDialectChanged, this, [this]() {
        setNeedsSave(m_settings->isSaveNeeded() || m_loadedBinaryDialect != m_optionsModel->binaryDialect());
        setRepresentsDefaults(m_settings->isDefaults() && m_optionsModel->binaryDialect() == KFormat::BinaryUnitDialect::IECBinaryDialect);
    });
}

void KCMRegionAndLang::save()
{
    if (settings()->isSaveNeeded()) {
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
                auto glibcLocale = GlibcLocaleConstructor::instance()->toGlibcLocale(lang);
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

    Q_EMIT saveClicked();
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

void KCMRegionAndLang::saveToConfigFile()
{
    KQuickManagedConfigModule::save();
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
        settings()->setLang(QString());
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
    return GlibcLocaleConstructor::instance()->enabled();
}

bool KCMRegionAndLang::isDefaults() const
{
    return m_settings->isDefaults() && m_optionsModel->binaryDialect() == KFormat::BinaryUnitDialect::IECBinaryDialect;
}

#include "kcmregionandlang.moc"
#include "moc_kcmregionandlang.cpp"
