// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once
#include <KService>
#include <QObject>
#include <QQmlEngine>

class ApplicationIntegration : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool calendarInstalled READ calendarInstalled NOTIFY calendarInstalledChanged)
    Q_PROPERTY(QString calendarApplicationName READ calendarApplicationName NOTIFY calendarApplicationNameChanged)

public:
    explicit ApplicationIntegration(QObject *parent = nullptr);
    ~ApplicationIntegration() = default;

    bool calendarInstalled() const;
    QString calendarApplicationName() const;
    Q_INVOKABLE void launchCalendar() const;

Q_SIGNALS:
    void calendarInstalledChanged();
    void calendarApplicationNameChanged();

private Q_SLOTS:
    void refresh();

private:
    KService::Ptr m_calendarService;
};
