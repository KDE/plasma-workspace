/*
    Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DRKONQI_STATUSNOTIFIER_H
#define DRKONQI_STATUSNOTIFIER_H

#include <QObject>

class QTimer;

class KStatusNotifierItem;

class CrashedApplication;

class StatusNotifier : public QObject
{
    Q_OBJECT

public:
    explicit StatusNotifier(QObject *parent = nullptr);
    ~StatusNotifier() override;

    void notify();

    static bool notificationServiceRegistered();

signals:
    void expired();
    void activated();

private:
    static bool canBeRestarted(CrashedApplication *app);

    QTimer *m_autoCloseTimer = nullptr;
    KStatusNotifierItem *m_sni = nullptr;

    QString m_title;
    QString m_iconName;

};

#endif // DRKONQI_STATUSNOTIFIER_H
