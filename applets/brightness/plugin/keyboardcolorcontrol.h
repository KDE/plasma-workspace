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
    Q_PROPERTY(bool accent READ isAccent WRITE setAccent NOTIFY accentChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit KeyboardColorControl(QObject *parent = nullptr);
    ~KeyboardColorControl() override;

    bool isSupported();
    bool isAccent();
    QString color();

Q_SIGNALS:
    void accentChanged();
    void colorChanged();

public Q_SLOTS:
    void setAccent(bool enabled);
    void setColor(const QString &color);

private:
    bool m_supported = false;
    bool m_accent = false;
    QString m_color = "white";

private Q_SLOTS:
    void slotAccentChanged(bool accent);
    void slotColorChanged(QString color);
};
