/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAction>
#include <QDateTime>
#include <QObject>
#include <QQmlEngine>

class ClipboardMenu : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Holds the IANA timezone ID (e.g. "Europe/London")
    Q_PROPERTY(QByteArray timezone READ timezone WRITE setTimezone NOTIFY timezoneChanged)
    Q_PROPERTY(bool secondsIncluded READ secondsIncluded WRITE setSecondsIncluded NOTIFY secondsIncludedChanged)

public:
    explicit ClipboardMenu(QObject *parent = nullptr);
    virtual ~ClipboardMenu();

    QByteArray timezone() const;
    void setTimezone(const QByteArray &timezone);

    bool secondsIncluded() const;
    void setSecondsIncluded(bool secondsIncluded);

    Q_INVOKABLE void setupMenu(QAction *action);

Q_SIGNALS:
    void timezoneChanged();
    void secondsIncludedChanged();

private:
    QByteArray m_timezone;
    bool m_secondsIncluded = false;
};
