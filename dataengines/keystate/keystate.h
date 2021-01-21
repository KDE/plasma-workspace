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

#ifndef KEYSTATEENGINE_H
#define KEYSTATEENGINE_H

#include <Plasma/DataEngine>

#include <kmodifierkeyinfo.h>

/**
 * This engine provides the current state of the keyboard modifiers
 * and mouse buttons, primarily useful for accessibility feature support.
 */
class KeyStatesEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    KeyStatesEngine(QObject *parent, const QVariantList &args);
    ~KeyStatesEngine() override;

    void init();
    Plasma::Service *serviceForSource(const QString &source) override;

protected:
    // bool sourceRequestEvent(const QString &name);
    // bool updateSourceEvent(const QString &source);

protected Q_SLOTS:
    void keyPressed(Qt::Key key, bool state);
    void keyLatched(Qt::Key key, bool state);
    void keyLocked(Qt::Key key, bool state);
    void mouseButtonPressed(Qt::MouseButton button, bool state);
    void keyAdded(Qt::Key key);
    void keyRemoved(Qt::Key key);

private:
    KModifierKeyInfo m_keyInfo;
    QMap<Qt::Key, QString> m_mods;
    QMap<Qt::MouseButton, QString> m_buttons;
};

#endif // KEYSTATEENGINE_H
