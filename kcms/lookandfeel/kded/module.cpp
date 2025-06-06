/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "module.h"
#include "settings.h"
#include "state.h"

#include <KPluginFactory>

#include <QDateTime>
#include <QDebug>
#include <QProcess>

K_PLUGIN_CLASS_WITH_JSON(AutoDarkLightLookAndFeel, "module.json")

AutoDarkLightLookAndFeel::AutoDarkLightLookAndFeel(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_settings(std::make_unique<Settings>())
    , m_state(std::make_unique<State>())
    , m_configWatcher(KConfigWatcher::create(m_settings->sharedConfig()))
{
    connect(m_configWatcher.get(), &KConfigWatcher::configChanged, this, &AutoDarkLightLookAndFeel::onConfigChanged);

    reconfigure();
}

AutoDarkLightLookAndFeel::~AutoDarkLightLookAndFeel()
{
}

bool AutoDarkLightLookAndFeel::changesConfig(const KConfigGroup &group, const QByteArrayList &names) const
{
    if (group.name() == QLatin1String("KDE")) {
        const QByteArrayList keys{
            QByteArrayLiteral("AutomaticDarkLightLookAndFeel"),
            QByteArrayLiteral("DefaultLightLookAndFeel"),
            QByteArrayLiteral("DefaultDarkLookAndFeel"),
        };
        return std::any_of(keys.begin(), keys.end(), [names](const QByteArray &name) {
            return names.contains(name);
        });
    }

    return false;
}

void AutoDarkLightLookAndFeel::onConfigChanged(const KConfigGroup &group, const QByteArrayList &names)
{
    if (changesConfig(group, names)) {
        m_settings->read();
        reconfigure();
    }
}

void AutoDarkLightLookAndFeel::reconfigure()
{
    if (!m_settings->automaticDarkLightLookAndFeel()) {
        m_scheduleTimer.reset();
        m_scheduleProvider.reset();
        m_skewMonitor.reset();
    } else {
        if (!m_skewMonitor) {
            m_skewMonitor = std::make_unique<KSystemClockSkewNotifier>();
            connect(m_skewMonitor.get(), &KSystemClockSkewNotifier::skewed, this, &AutoDarkLightLookAndFeel::reschedule);
            m_skewMonitor->setActive(true);
        }

        if (!m_scheduleTimer) {
            m_scheduleTimer = std::make_unique<QTimer>();
            m_scheduleTimer->setSingleShot(true);
            connect(m_scheduleTimer.get(), &QTimer::timeout, this, &AutoDarkLightLookAndFeel::reschedule);
        }

        if (!m_scheduleProvider) {
            m_scheduleProvider = std::make_unique<KDarkLightScheduleProvider>(m_state->serializedSchedule());
            connect(m_scheduleProvider.get(), &KDarkLightScheduleProvider::scheduleChanged, this, [this]() {
                m_state->setSerializedSchedule(m_scheduleProvider->state());
                m_state->save();

                reschedule();
            });
        }

        reschedule();
    }
}

void AutoDarkLightLookAndFeel::reschedule()
{
    const QDateTime now = QDateTime::currentDateTime();

    const KDarkLightSchedule schedule = m_scheduleProvider->schedule();
    m_previousTransition = schedule.previousTransition(now);
    m_nextTransition = schedule.nextTransition(now);

    QDateTime rescheduleDateTime;
    if (m_previousTransition->test(now) != KDarkLightTransition::Passed) {
        rescheduleDateTime = m_previousTransition->endDateTime();
    } else {
        rescheduleDateTime = m_nextTransition->endDateTime();
    }
    m_scheduleTimer->start(rescheduleDateTime - now);

    recolor();
}

void AutoDarkLightLookAndFeel::recolor()
{
    LookAndFeelVariant variant;
    switch (m_previousTransition->test(QDateTime::currentDateTime())) {
    case KDarkLightTransition::Upcoming:
    case KDarkLightTransition::InProgress:
        if (m_previousTransition->type() == KDarkLightTransition::Morning) {
            variant = LookAndFeelVariant::Dark;
        } else {
            variant = LookAndFeelVariant::Light;
        }
        break;
    case KDarkLightTransition::Passed:
        if (m_previousTransition->type() == KDarkLightTransition::Morning) {
            variant = LookAndFeelVariant::Light;
        } else {
            variant = LookAndFeelVariant::Dark;
        }
        break;
    }

    const QString lookAndFeelPackage = variant == LookAndFeelVariant::Dark ? m_settings->defaultDarkLookAndFeel() : m_settings->defaultLightLookAndFeel();
    QProcess::startDetached(QStringLiteral("plasma-apply-lookandfeel"), QStringList({QStringLiteral("-a"), lookAndFeelPackage}));
}

#include "moc_module.cpp"
#include "module.moc"
