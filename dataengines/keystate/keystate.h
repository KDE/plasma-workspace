/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma/DataEngine>

#include <KLazyLocalizedString>
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
    QMap<Qt::Key, KLazyLocalizedString> m_mods;
    QMap<Qt::MouseButton, KLazyLocalizedString> m_buttons;
};
