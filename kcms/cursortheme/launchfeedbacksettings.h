/*
    SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LAUNCHFEEDBACKSETTINGS_H
#define LAUNCHFEEDBACKSETTINGS_H

#include "launchfeedbacksettingsbase.h"

class LaunchFeedbackSettingsStore;

class LaunchFeedbackSettings : public LaunchFeedbackSettingsBase
{
    Q_OBJECT

    Q_PROPERTY(CursorFeedbackType cursorFeedbackType READ cursorFeedbackType WRITE setCursorFeedbackType NOTIFY cursorFeedbackTypeChanged)

public:
    enum CursorFeedbackType {
        CNone = 0,
        CStatic,
        CBlinking,
        CBouncing
    };
    Q_ENUM(CursorFeedbackType)

    LaunchFeedbackSettings(QObject *parent = nullptr);

    CursorFeedbackType cursorFeedbackType() const;
    void setCursorFeedbackType(CursorFeedbackType type);
    Q_SIGNAL void cursorFeedbackTypeChanged();

private:
    LaunchFeedbackSettingsStore *m_settingsStore;

    using NotifySignalType = void (LaunchFeedbackSettings::*)();
    void addItemInternal(const QByteArray &propertyName, const QVariant &defaultValue, NotifySignalType notifySignal);
};

#endif // LAUNCHFEEDBACKSETTINGS_H
