/*
 * Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CLIPBOARDMENU_H
#define CLIPBOARDMENU_H

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
    void setSecondsIncluded(const bool &secondsIncluded);

    Q_INVOKABLE void setupMenu(QAction *action);

signals:
    void currentDateChanged();
    void secondsIncludedChanged();


private:
    QDateTime m_currentDate;
    bool m_secondsIncluded = false;
};

#endif // CLIPBOARDMENU_H
