/*
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QBindable>
#include <QObject>
#include <qqmlregistration.h>
#include <qtmetamacros.h>

class ColorBlindnessCorrectionControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool supported READ isSupported)
    Q_PROPERTY(bool shown READ isShown)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
    explicit ColorBlindnessCorrectionControl(QObject *parent = nullptr);
    ~ColorBlindnessCorrectionControl() override;

    bool isSupported();
    bool isShown();
    bool isEnabled();

public Q_SLOTS:
    void setEnabled(bool enabled);

private:
    bool m_supported = false;
    bool m_shown = false;
    bool m_enabled = false;

    void updateProperties(const QVariantMap &properties);

private Q_SLOTS:
    void handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
};
