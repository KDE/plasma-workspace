/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "keystate.h"

#include <klocalizedstring.h>
#include <kmodifierkeyinfo.h>
#include "keyservice.h"

KeyStatesEngine::KeyStatesEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    m_mods.insert(Qt::Key_Shift, I18N_NOOP("Shift"));
    m_mods.insert(Qt::Key_Control, I18N_NOOP("Ctrl"));
    m_mods.insert(Qt::Key_Alt, I18N_NOOP("Alt"));
    m_mods.insert(Qt::Key_Meta, I18N_NOOP("Meta"));
    m_mods.insert(Qt::Key_Super_L, I18N_NOOP("Super"));
    m_mods.insert(Qt::Key_Hyper_L, I18N_NOOP("Hyper"));
    m_mods.insert(Qt::Key_AltGr, I18N_NOOP("AltGr"));
    m_mods.insert(Qt::Key_NumLock, I18N_NOOP("Num Lock"));
    m_mods.insert(Qt::Key_CapsLock, I18N_NOOP("Caps Lock"));
    m_mods.insert(Qt::Key_ScrollLock, I18N_NOOP("Scroll Lock"));

    m_buttons.insert(Qt::LeftButton, I18N_NOOP("Left Button"));
    m_buttons.insert(Qt::RightButton, I18N_NOOP("Right Button"));
    m_buttons.insert(Qt::MiddleButton, I18N_NOOP("Middle Button"));
    m_buttons.insert(Qt::XButton1, I18N_NOOP("First X Button"));
    m_buttons.insert(Qt::XButton2, I18N_NOOP("Second X Button"));
    init();
}

KeyStatesEngine::~KeyStatesEngine()
{
}

void KeyStatesEngine::init()
{
    QMap<Qt::Key, QString>::const_iterator it;
    QMap<Qt::Key, QString>::const_iterator end = m_mods.constEnd();
    for (it = m_mods.constBegin(); it != end; ++it) {
        if (m_keyInfo.knowsKey(it.key())) {
            Data data;
            data.insert(I18N_NOOP("Pressed"), m_keyInfo.isKeyPressed(it.key()));
            data.insert(I18N_NOOP("Latched"), m_keyInfo.isKeyLatched(it.key()));
            data.insert(I18N_NOOP("Locked"), m_keyInfo.isKeyLocked(it.key()));
            setData(it.value(), data);
        }
    }

    QMap<Qt::MouseButton, QString>::const_iterator it2;
    QMap<Qt::MouseButton, QString>::const_iterator end2 = m_buttons.constEnd();
    for (it2 = m_buttons.constBegin(); it2 != end2; ++it2) {
        Data data;
        data.insert(I18N_NOOP("Pressed"), m_keyInfo.isButtonPressed(it2.key()));
        setData(it2.value(), data);
    }

    connect(&m_keyInfo, &KModifierKeyInfo::keyPressed, this, &KeyStatesEngine::keyPressed);
    connect(&m_keyInfo, &KModifierKeyInfo::keyLatched, this, &KeyStatesEngine::keyLatched);
    connect(&m_keyInfo, &KModifierKeyInfo::keyLocked, this, &KeyStatesEngine::keyLocked);
    connect(&m_keyInfo, &KModifierKeyInfo::buttonPressed, this, &KeyStatesEngine::mouseButtonPressed);
    connect(&m_keyInfo, &KModifierKeyInfo::keyAdded, this, &KeyStatesEngine::keyAdded);
    connect(&m_keyInfo, &KModifierKeyInfo::keyRemoved, this, &KeyStatesEngine::keyRemoved);
}

Plasma::Service *KeyStatesEngine::serviceForSource(const QString &source)
{
    QMap<Qt::Key, QString>::const_iterator it;
    QMap<Qt::Key, QString>::const_iterator end = m_mods.constEnd();
    for (it = m_mods.constBegin(); it != end; ++it) {
        if (it.value() == source) {
            return new KeyService(this, &m_keyInfo, it.key());
        }
    }

    return Plasma::DataEngine::serviceForSource(source);
}

void KeyStatesEngine::keyPressed(Qt::Key key, bool state)
{
    if (m_mods.contains(key)) {
        setData(m_mods.value(key), I18N_NOOP("Pressed"), state);
    }
}

void KeyStatesEngine::keyLatched(Qt::Key key, bool state)
{
    if (m_mods.contains(key)) {
        setData(m_mods.value(key), I18N_NOOP("Latched"), state);
    }
}

void KeyStatesEngine::keyLocked(Qt::Key key, bool state)
{
    if (m_mods.contains(key)) {
        setData(m_mods.value(key), I18N_NOOP("Locked"), state);
    }
}

void KeyStatesEngine::mouseButtonPressed(Qt::MouseButton button, bool state)
{
    if (m_buttons.contains(button)) {
        setData(m_buttons.value(button), I18N_NOOP("Pressed"), state);
    }
}

void KeyStatesEngine::keyAdded(Qt::Key key)
{
    if (m_mods.contains(key)) {
        Data data;
        data.insert(I18N_NOOP("Pressed"), m_keyInfo.isKeyPressed(key));
        data.insert(I18N_NOOP("Latched"), m_keyInfo.isKeyLatched(key));
        data.insert(I18N_NOOP("Locked"), m_keyInfo.isKeyLocked(key));
        setData(m_mods.value(key), data);
    }
}

void KeyStatesEngine::keyRemoved(Qt::Key key)
{
    if (m_mods.contains(key)) {
        removeSource(m_mods.value(key));
    }
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(keystate, KeyStatesEngine, "plasma-dataengine-keystate.json")

#include "keystate.moc"
