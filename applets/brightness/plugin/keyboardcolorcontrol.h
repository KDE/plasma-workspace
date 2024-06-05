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

    Q_PROPERTY(bool supported READ isSupported CONSTANT FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)

public:
    explicit KeyboardColorControl(QObject *parent = nullptr);
    ~KeyboardColorControl() override;

    bool isSupported() const;

    bool enabled() const;
    void setEnabled(bool enabled);

Q_SIGNALS:
    void enabledChanged();

private:
    bool m_supported = false;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(KeyboardColorControl, bool, m_enabled, false, &KeyboardColorControl::enabledChanged)
};
