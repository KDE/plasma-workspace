/*
    kcmregionandlang.h
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <optional>
#include <unordered_map>

#include "settingtype.h"

#include <KConfigGroup>
#include <KQuickAddons/ManagedConfigModule>

class RegionAndLangSettings;
class OptionsModel;
class LocaleGeneratorBase;

class KCMRegionAndLang : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(RegionAndLangSettings *settings READ settings CONSTANT)
    Q_PROPERTY(OptionsModel *optionsModel READ optionsModel CONSTANT)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)

public:
    explicit KCMRegionAndLang(QObject *parent, const KPluginMetaData &data, const QVariantList &list = QVariantList());
    void save() override;

    RegionAndLangSettings *settings() const;

    OptionsModel *optionsModel() const;
    bool enabled() const;
    static bool isGlibc();
    std::optional<QString> toGlibcLocale(const QString &lang);
    Q_INVOKABLE void unset(KCM_RegionAndLang::SettingType setting);
    Q_INVOKABLE void reboot();
Q_SIGNALS:
    void saveClicked();
    void takeEffectNextTime();
    void startGenerateLocale();
    void generateFinished();
    void requireInstallFont();
    void enabledChanged();
    void encountedError(const QString &reason);
    void userHasToGenerateManually(const QString &reason);

private Q_SLOTS:
    void saveToConfigFile();

private:
    std::unordered_map<QString, QString> constructGlibcLocaleMap();
    static QString failedFindLocalesMessage();
    static QString localeFileDirPath();
    static QString toUTF8Locale(const QString &locale);

    QHash<QString, QString> m_cachedFlags;

    RegionAndLangSettings *m_settings;
    OptionsModel *m_optionsModel;
    LocaleGeneratorBase *m_generator = nullptr;
    QProcess *m_localectl = nullptr;
    bool m_enabled = false;
};
