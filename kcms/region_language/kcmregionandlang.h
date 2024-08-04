/*
    kcmregionandlang.h
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>
    SPDX-FileCopyrightText: 2023 Serenity Cybersecurity, LLC <license@futurecrew.ru>
                                 Author: Gleb Popov <arrowd@FreeBSD.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <optional>
#include <unordered_map>

#include "config-workspace.h"
#include "settingtype.h"

#include <KConfigGroup>
#include <KQuickManagedConfigModule>
#include <QProcess>

class RegionAndLangSettings;
class OptionsModel;
class LocaleGeneratorBase;
class QDBusPendingCallWatcher;

class KCMRegionAndLang : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(RegionAndLangSettings *settings READ settings CONSTANT)
    Q_PROPERTY(OptionsModel *optionsModel READ optionsModel CONSTANT)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(bool localedAvailable READ localedAvailable NOTIFY localedAvailableChanged)

public:
    explicit KCMRegionAndLang(QObject *parent, const KPluginMetaData &data);
    void save() override;
    void load() override;
    void defaults() override;
    bool isDefaults() const override;

    RegionAndLangSettings *settings() const;

    OptionsModel *optionsModel() const;
    bool enabled() const;
    bool localedAvailable() const;
#ifdef GLIBC_LOCALE
    std::optional<QString> toGlibcLocale(const QString &lang);
#endif
    Q_INVOKABLE void unset(KCM_RegionAndLang::SettingType setting) const;
    Q_INVOKABLE void reboot();
    Q_INVOKABLE void applyToLocal();
    Q_INVOKABLE void applyToSystem();
    Q_INVOKABLE void saveCanceled();
    Q_INVOKABLE QQuickItem *currentPage();

Q_SIGNALS:
    void saveClicked();
    void loadClicked();
    void defaultsClicked();
    void takeEffectNextTime();
    void startGenerateLocale();
    void generateFinished();
    void requireInstallFont();
    void enabledChanged();
    void localedAvailableChanged();
    void encountedError(const QString &reason);
    void userHasToGenerateManually(const QString &reason);

private Q_SLOTS:
    void saveToConfigFile();
    void handleLocaledAvailable(QDBusPendingCallWatcher *);

private:
#ifdef GLIBC_LOCALE
    std::unordered_map<QString, QString> constructGlibcLocaleMap();
#endif
    // try to get system wide locale settings via localed, only works if m_localedAvailable is true
    void getSystemLocale(const std::function<void(const QStringList &)> &);
    void setSystemLocale(const QStringList &locale);
    static QString failedFindLocalesMessage();
    static QString localeFileDirPath();
    static QString toUTF8Locale(const QString &locale);

    RegionAndLangSettings *m_settings;
    OptionsModel *m_optionsModel;
    LocaleGeneratorBase *m_generator = nullptr;
    QProcess *m_localectl = nullptr;
    bool m_enabled = false;
    int m_loadedBinaryDialect;
    bool m_localedAvailable = false;
};
