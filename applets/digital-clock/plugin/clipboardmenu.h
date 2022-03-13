/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QDateTime>
#include <QObject>

class QAction;

class ClipboardMenu : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDateTime currentDate READ currentDate WRITE setCurrentDate NOTIFY currentDateChanged)
    Q_PROPERTY(bool secondsIncluded READ secondsIncluded WRITE setSecondsIncluded NOTIFY secondsIncludedChanged)

public:
    explicit ClipboardMenu(QObject *parent = nullptr);
    virtual ~ClipboardMenu();

    QDateTime currentDate() const;
    void setCurrentDate(const QDateTime &date);

    bool secondsIncluded() const;
    void setSecondsIncluded(bool secondsIncluded);

    Q_INVOKABLE void setupMenu(QAction *action);

Q_SIGNALS:
    void currentDateChanged();
    void secondsIncludedChanged();

private:
    QDateTime m_currentDate;
    bool m_secondsIncluded = false;
};
