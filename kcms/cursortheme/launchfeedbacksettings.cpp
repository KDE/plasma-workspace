/*
    SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "launchfeedbacksettings.h"

class LaunchFeedbackSettingsStore : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busyCursorDisabled READ busyCursorDisabled WRITE setBusyCursorDisabled)
    Q_PROPERTY(bool busyCursorStatic READ busyCursorStatic WRITE setBusyCursorStatic)
    Q_PROPERTY(bool busyCursorBlinking READ busyCursorBlinking WRITE setBusyCursorBlinking)
    Q_PROPERTY(bool busyCursorBouncing READ busyCursorBouncing WRITE setBusyCursorBouncing)

public:
    LaunchFeedbackSettingsStore(LaunchFeedbackSettings *parent = nullptr)
        : QObject(parent)
        , m_settings(parent)
    {
        setBusyCursorDisabled(!m_settings->busyCursor() && !m_settings->blinking() && !m_settings->bouncing());
        setBusyCursorStatic(m_settings->busyCursor() && !m_settings->blinking() && !m_settings->bouncing());
        setBusyCursorBlinking(m_settings->busyCursor() && m_settings->blinking() && !m_settings->bouncing());
        setBusyCursorBouncing(m_settings->busyCursor() && !m_settings->blinking() && m_settings->bouncing());
    }

    void setBusyCursorDisabled(bool busyCursorDisabled)
    {
        m_busyCursorDisabled = busyCursorDisabled;
    }
    bool busyCursorDisabled() const
    {
        return m_busyCursorDisabled;
    }
    bool busyCursorDisabledDefault() const
    {
        return !m_settings->defaultBusyCursorValue() && !m_settings->defaultBlinkingValue() && !m_settings->defaultBouncingValue();
    }

    void setBusyCursorStatic(bool busyCursorStatic)
    {
        m_busyCursorStatic = busyCursorStatic;
    }
    bool busyCursorStatic() const
    {
        return m_busyCursorStatic;
    }
    bool busyCursorStaticDefault()
    {
        return m_settings->defaultBusyCursorValue() && !m_settings->defaultBlinkingValue() && !m_settings->defaultBouncingValue();
    }

    void setBusyCursorBlinking(bool busyCursorBlinking)
    {
        m_busyCursorBlinking = busyCursorBlinking;
    }
    bool busyCursorBlinking() const
    {
        return m_busyCursorBlinking;
    }
    bool busyCursorBlinkingDefault()
    {
        return m_settings->defaultBusyCursorValue() && m_settings->defaultBlinkingValue() && !m_settings->defaultBouncingValue();
    }

    void setBusyCursorBouncing(bool busyCursorBouncing)
    {
        m_busyCursorBouncing = busyCursorBouncing;
    }
    bool busyCursorBouncing() const
    {
        return m_busyCursorBouncing;
    }
    bool busyCursorBouncingcDefault()
    {
        return m_settings->defaultBusyCursorValue() && !m_settings->defaultBlinkingValue() && m_settings->defaultBouncingValue();
    }

private:
    LaunchFeedbackSettings *m_settings;
    bool m_busyCursorDisabled;
    bool m_busyCursorStatic;
    bool m_busyCursorBlinking;
    bool m_busyCursorBouncing;
};

LaunchFeedbackSettings::LaunchFeedbackSettings(QObject *parent)
    : LaunchFeedbackSettingsBase(parent)
    , m_settingsStore(new LaunchFeedbackSettingsStore(this))
{
    addItemInternal("busyCursorDisabled", m_settingsStore->busyCursorDisabledDefault(), &LaunchFeedbackSettings::cursorFeedbackTypeChanged);
    addItemInternal("busyCursorStatic", m_settingsStore->busyCursorStaticDefault(), &LaunchFeedbackSettings::cursorFeedbackTypeChanged);
    addItemInternal("busyCursorBlinking", m_settingsStore->busyCursorBlinkingDefault(), &LaunchFeedbackSettings::cursorFeedbackTypeChanged);
    addItemInternal("busyCursorBouncing", m_settingsStore->busyCursorBouncingcDefault(), &LaunchFeedbackSettings::cursorFeedbackTypeChanged);
}

void LaunchFeedbackSettings::addItemInternal(const QByteArray &propertyName, const QVariant &defaultValue, NotifySignalType notifySignal)
{
    auto item = new KPropertySkeletonItem(m_settingsStore, propertyName, defaultValue);
    addItem(item, propertyName);
    item->setNotifyFunction([this, notifySignal] {
        Q_EMIT(this->*notifySignal)();
    });
}

LaunchFeedbackSettings::CursorFeedbackType LaunchFeedbackSettings::cursorFeedbackType() const
{
    if (findItem("busyCursorDisabled")->property().toBool()) {
        return CursorFeedbackType::CNone;
    } else if (findItem("busyCursorStatic")->property().toBool()) {
        return CursorFeedbackType::CStatic;
    } else if (findItem("busyCursorBlinking")->property().toBool()) {
        return CursorFeedbackType::CBlinking;
    } else if (findItem("busyCursorBouncing")->property().toBool()) {
        return CursorFeedbackType::CBouncing;
    }

    return CursorFeedbackType::CBouncing;
}

void LaunchFeedbackSettings::setCursorFeedbackType(CursorFeedbackType type)
{
    findItem("busyCursorDisabled")->setProperty(type == CursorFeedbackType::CNone);
    findItem("busyCursorStatic")->setProperty(type == CursorFeedbackType::CStatic);
    findItem("busyCursorBlinking")->setProperty(type == CursorFeedbackType::CBlinking);
    findItem("busyCursorBouncing")->setProperty(type == CursorFeedbackType::CBouncing);

    Q_EMIT cursorFeedbackTypeChanged();
    Q_EMIT configChanged();
}

#include "launchfeedbacksettings.moc"
