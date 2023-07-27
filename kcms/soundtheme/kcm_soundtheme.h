/*
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KQuickManagedConfigModule>

#include "soundthemesettings.h"

class SoundThemeData;
struct ca_context;

class ThemeInfo : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(QString comment MEMBER comment CONSTANT)
    Q_PROPERTY(QStringList inherits MEMBER inherits CONSTANT)
    Q_PROPERTY(QStringList directories MEMBER directories CONSTANT)
    Q_PROPERTY(bool hidden MEMBER isHidden CONSTANT)
    Q_PROPERTY(QString example MEMBER example CONSTANT)

public:
    explicit ThemeInfo(const QString &themeId, QObject *parent = nullptr);

public:
    // Properties provided by index.theme
    QString id;
    QString name;
    QString comment;
    QStringList inherits;
    QStringList directories;
    bool isHidden;
    QString example;
    // Validation
    bool isValid = false;
};

class KCMSoundTheme : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(SoundThemeSettings *settings READ settings CONSTANT);
    Q_PROPERTY(QList<ThemeInfo *> themes MEMBER m_themes NOTIFY themesLoaded);
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY themeChanged);
    Q_PROPERTY(QString playingTheme MEMBER m_playingTheme NOTIFY playingChanged);
    Q_PROPERTY(QString playingSound MEMBER m_playingSound NOTIFY playingChanged);

public:
    KCMSoundTheme(QObject *parent, const KPluginMetaData &data);
    ~KCMSoundTheme() override;

    SoundThemeSettings *settings() const;
    int currentIndex() const;

    Q_INVOKABLE int indexOf(const QString &themeId) const;
    Q_INVOKABLE QString nameFor(const QString &themeId) const;

    Q_INVOKABLE int playSound(const QString &themeId, const QStringList &soundList);
    Q_INVOKABLE void cancelSound();

    virtual void load() override;

Q_SIGNALS:
    void themesLoaded();
    void themeChanged();
    void playingChanged();

private:
    ca_context *canberraContext();
    static void ca_finish_callback(ca_context *c, uint32_t id, int error_code, void *userdata);
    Q_SLOT void onPlayingFinished();

    void loadThemes();

private:
    ca_context *m_canberraContext = nullptr;
    SoundThemeData *m_data = nullptr;

    QList<ThemeInfo *> m_themes;
    QString m_playingTheme;
    QString m_playingSound;
};
