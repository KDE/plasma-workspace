/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "keystate.h"

#include <KModifierKeyInfo>

#include "keyboardindicator_debug.h"

namespace
{
[[nodiscard]] std::shared_ptr<KModifierKeyInfo> keyInfo()
{
    static std::weak_ptr<KModifierKeyInfo> s_backend;
    if (s_backend.expired()) {
        std::shared_ptr<KModifierKeyInfo> ptr{new KModifierKeyInfo};
        s_backend = ptr;
        return ptr;
    }
    return s_backend.lock();
}
}

KeyState::KeyState(QObject *parent)
    : QObject(parent)
{
}

KeyState::~KeyState()
{
}

Qt::Key KeyState::key() const
{
    return m_key;
}

void KeyState::setKey(Qt::Key newKey)
{
    if (m_key == newKey) [[unlikely]] {
        return;
    }

    m_key = newKey;
    Q_EMIT keyChanged();

    if (m_keyInfo) {
        if (!m_keyInfo->knowsKey(newKey)) [[unlikely]] {
            // Ignore unknown keys
            qCWarning(KEYBOARDINDICATOR_DEBUG) << "Unknown key" << newKey;
            m_keyInfo->disconnect(this);
            m_keyInfo.reset();
            return;
        }
    } else {
        m_keyInfo = keyInfo();
        if (!m_keyInfo->knowsKey(newKey)) [[unlikely]] {
            qCWarning(KEYBOARDINDICATOR_DEBUG) << "Unknown key" << newKey;
            m_keyInfo.reset();
            return;
        }
    }

    m_pressed = m_keyInfo->isKeyPressed(m_key);
    m_latched = m_keyInfo->isKeyLatched(m_key);
    m_locked = m_keyInfo->isKeyLocked(m_key);

    connect(m_keyInfo.get(), &KModifierKeyInfo::keyPressed, this, &KeyState::onKeyPressed);
    connect(m_keyInfo.get(), &KModifierKeyInfo::keyLatched, this, &KeyState::onKeyLatched);
    connect(m_keyInfo.get(), &KModifierKeyInfo::keyLocked, this, &KeyState::onKeyLocked);
    connect(m_keyInfo.get(), &KModifierKeyInfo::keyAdded, this, &KeyState::onKeyAdded);
    connect(m_keyInfo.get(), &KModifierKeyInfo::keyRemoved, this, &KeyState::onKeyRemoved);
}

bool KeyState::pressed() const
{
    return m_pressed;
}

bool KeyState::latched() const
{
    return m_latched;
}

bool KeyState::locked() const
{
    return m_locked;
}

void KeyState::lock(bool lock)
{
    if (m_keyInfo) [[likely]] {
        m_keyInfo->setKeyLocked(m_key, lock);
    }
}

void KeyState::latch(bool latch)
{
    if (m_keyInfo) [[likely]] {
        m_keyInfo->setKeyLatched(m_key, latch);
    }
}

void KeyState::onKeyPressed(Qt::Key key, bool state)
{
    if (key == m_key) {
        m_pressed = state;
    }
}

void KeyState::onKeyLatched(Qt::Key key, bool state)
{
    if (key == m_key) {
        m_latched = state;
    }
}

void KeyState::onKeyLocked(Qt::Key key, bool state)
{
    if (key == m_key) {
        m_locked = state;
    }
}

void KeyState::onKeyAdded(Qt::Key key)
{
    if (key == m_key) {
        m_pressed = m_keyInfo->isKeyPressed(m_key);
        m_latched = m_keyInfo->isKeyLatched(m_key);
        m_locked = m_keyInfo->isKeyLocked(m_key);
    }
}

void KeyState::onKeyRemoved(Qt::Key key)
{
    if (key == m_key) {
        m_pressed = false;
        m_latched = false;
        m_locked = false;
    }
}

MouseButtonState::MouseButtonState(QObject *parent)
    : QObject(parent)
{
}

MouseButtonState::~MouseButtonState()
{
}

Qt::MouseButton MouseButtonState::button() const
{
    return m_button;
}

void MouseButtonState::setButton(Qt::MouseButton newButton)
{
    if (m_button == newButton) [[unlikely]] {
        return;
    }

    m_button = newButton;
    Q_EMIT buttonChanged();

    if (m_button <= Qt::NoButton || m_button > Qt::MaxMouseButton) {
        if (m_keyInfo) {
            qCWarning(KEYBOARDINDICATOR_DEBUG) << "Unknown mouse button" << newButton;
            m_keyInfo->disconnect(this);
            m_keyInfo.reset();
        }
        return;
    }

    if (!m_keyInfo) {
        m_keyInfo = keyInfo();
    }

    m_pressed = m_keyInfo->isButtonPressed(m_button);

    connect(m_keyInfo.get(), &KModifierKeyInfo::buttonPressed, this, &MouseButtonState::onMouseButtonPressed);
}

bool MouseButtonState::pressed() const
{
    return m_pressed;
}

void MouseButtonState::onMouseButtonPressed(Qt::MouseButton mouseButton, bool state)
{
    if (mouseButton == m_button) {
        m_pressed = state;
    }
}

#include "moc_keystate.cpp"
