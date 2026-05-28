/*
    SPDX-FileCopyrightText: 2026 Martin Riethmayer <ripper@freakmail.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfigGroup>
#include <QObject>
#include <QSharedPointer>
#include <QtQml>

class KConfigWatcher;

class AccessibilityStatus : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool slowKeysEnabled READ slowKeysEnabled NOTIFY slowKeysEnabledChanged)

public:
    explicit AccessibilityStatus(QObject *parent = nullptr);
    ~AccessibilityStatus() override = default;

    bool slowKeysEnabled() const;

private Q_SLOTS:
    void configChanged(const KConfigGroup &group);

Q_SIGNALS:
    void slowKeysEnabledChanged();

private:
    bool checkSlowKeyStatus(const KConfigGroup &group) const;

    QSharedPointer<KConfigWatcher> m_configWatcher;
    bool m_slowKeysEnabled = false;
};
