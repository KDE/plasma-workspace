/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <qqmlregistration.h>

#include "virtualkeyboard_interface.h"

class KwinVirtualKeyboardInterface : public OrgKdeKwinVirtualKeyboardInterface
{
    Q_OBJECT
    QML_NAMED_ELEMENT(KWinVirtualKeyboard)
    QML_SINGLETON

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool activeClientSupportsTextInput READ activeClientSupportsTextInput NOTIFY activeClientSupportsTextInputChanged)
    Q_PROPERTY(bool willShowOnActive READ willShowOnActive NOTIFY willShowOnActiveChanged)

public:
    enum class VirtualKeyboardVisibility {
        Never,
        NonMouseInput,
        AnyInput,
    };
    Q_ENUM(VirtualKeyboardVisibility);

    Q_INVOKABLE void forceActivate();

    bool willShowOnActive() const;

    KwinVirtualKeyboardInterface();

Q_SIGNALS:
    void willShowOnActiveChanged();

private:
    /*!
     * Re-fetches willShowOnActive over D-Bus since we don't get a signal for it, and we need to
     * update it when any of the other properties change.
     */
    void refreshWillShowOnActive();

    bool m_willShowOnActive = false;
};
