/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfigWatcher>
#include <KDEDModule>
#include <KDarkLightScheduleProvider>
#include <KSystemClockSkewNotifier>

#include <QTimer>

class Settings;
class State;

enum class LookAndFeelVariant {
    Dark,
    Light,
};

class AutoDarkLightLookAndFeel : public KDEDModule
{
    Q_OBJECT

public:
    explicit AutoDarkLightLookAndFeel(QObject *parent = nullptr, const QList<QVariant> &arguments = {});
    ~AutoDarkLightLookAndFeel() override;

    void reconfigure();
    void reschedule();
    void recolor();

private:
    bool changesConfig(const KConfigGroup &group, const QByteArrayList &names) const;
    void onConfigChanged(const KConfigGroup &group, const QByteArrayList &names);

    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<State> m_state;
    KConfigWatcher::Ptr m_configWatcher;

    std::unique_ptr<KDarkLightScheduleProvider> m_scheduleProvider;
    std::unique_ptr<KSystemClockSkewNotifier> m_skewMonitor;
    std::unique_ptr<QTimer> m_scheduleTimer;

    std::optional<KDarkLightTransition> m_previousTransition;
    std::optional<KDarkLightTransition> m_nextTransition;
};
