/*
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QBindable>
#include <QObject>
#include <qqmlregistration.h>

class KeyboardColorControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool supported READ isSupported)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
    explicit KeyboardColorControl(QObject *parent = nullptr);
    ~KeyboardColorControl() override;

    bool isSupported();
    bool isEnabled();

public Q_SLOTS:
    void setEnabled(bool enabled);

private:
    bool m_supported = false;
    bool m_enabled = false;
};
