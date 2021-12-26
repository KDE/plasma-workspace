// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once
#include <KService>
#include <QObject>

class ApplicationIntegration : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool calendarInstalled READ calendarInstalled CONSTANT)

public:
    explicit ApplicationIntegration(QObject *parent = nullptr);
    ~ApplicationIntegration() = default;

    bool calendarInstalled() const;
    Q_INVOKABLE void launchCalendar() const;

private:
    KService::Ptr m_calendarService;
};
