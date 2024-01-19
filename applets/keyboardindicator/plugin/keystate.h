/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QBindable>
#include <QObject>
#include <qqmlregistration.h>

class KModifierKeyInfo;

/**
 * This QML object provides the current state of the keyboard modifiers,
 * primarily useful for accessibility feature support.
 */
class KeyState : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /**
     * @param key a modifier key
     */
    Q_PROPERTY(Qt::Key key READ key WRITE setKey NOTIFY keyChanged)

    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged)
    Q_PROPERTY(bool latched READ latched NOTIFY latchedChanged)
    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)

public:
    explicit KeyState(QObject *parent = nullptr);
    ~KeyState() override;

    Qt::Key key() const;
    void setKey(Qt::Key newKey);

    bool pressed() const;
    bool latched() const;
    bool locked() const;

    /// Unsupported in Wayland
    Q_INVOKABLE void lock(bool lock);
    /// Unsupported in Wayland
    Q_INVOKABLE void latch(bool latch);

Q_SIGNALS:
    void keyChanged();
    void pressedChanged();
    void latchedChanged();
    void lockedChanged();

private Q_SLOTS:
    void onKeyPressed(Qt::Key key, bool state);
    void onKeyLatched(Qt::Key key, bool state);
    void onKeyLocked(Qt::Key key, bool state);
    void onKeyAdded(Qt::Key key);
    void onKeyRemoved(Qt::Key key);

private:
    std::shared_ptr<KModifierKeyInfo> m_keyInfo;
    Qt::Key m_key = Qt::Key_Any;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(KeyState, bool, m_pressed, false, &KeyState::pressedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(KeyState, bool, m_latched, false, &KeyState::latchedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(KeyState, bool, m_locked, false, &KeyState::lockedChanged)
};

/**
 * This QML object provides the current state of the mouse buttons,
 * primarily useful for accessibility feature support.
 */
class MouseButtonState : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Qt::MouseButton button READ button WRITE setButton NOTIFY buttonChanged)

    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged)

public:
    explicit MouseButtonState(QObject *parent = nullptr);
    ~MouseButtonState() override;

    Qt::MouseButton button() const;
    void setButton(Qt::MouseButton newButton);

    bool pressed() const;

Q_SIGNALS:
    void buttonChanged();
    void pressedChanged();

private Q_SLOTS:
    void onMouseButtonPressed(Qt::MouseButton mouseButton, bool state);

private:
    std::shared_ptr<KModifierKeyInfo> m_keyInfo;
    Qt::MouseButton m_button = Qt::NoButton;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MouseButtonState, bool, m_pressed, false, &MouseButtonState::pressedChanged)
};
