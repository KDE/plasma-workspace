/*
 * SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-only OR GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SOUNDTHEMECONFIG_H
#define SOUNDTHEMECONFIG_H

#include <QObject>

#include <KConfigWatcher>

class SoundThemeConfig : public QObject
{
    Q_OBJECT

public:
    explicit SoundThemeConfig(QObject *parent = nullptr);

    QString soundTheme() const;

Q_SIGNALS:
    void soundThemeChanged(const QString &theme);

private Q_SLOTS:
    void kdeglobalsChanged(const KConfigGroup &group, const QByteArrayList &names);

private:
    QString m_soundTheme;
    KConfigWatcher::Ptr m_soundThemeWatcher;
};

#endif
