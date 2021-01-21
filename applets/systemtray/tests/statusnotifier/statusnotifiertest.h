/******************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                           *
 *                                                                             *
 *   This library is free software; you can redistribute it and/or             *
 *   modify it under the terms of the GNU Library General Public               *
 *   License as published by the Free Software Foundation; either              *
 *   version 2 of the License, or (at your option) any later version.          *
 *                                                                             *
 *   This library is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
 *   Library General Public License for more details.                          *
 *                                                                             *
 *   You should have received a copy of the GNU Library General Public License *
 *   along with this library; see the file COPYING.LIB.  If not, write to      *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 *   Boston, MA 02110-1301, USA.                                               *
 *******************************************************************************/

#ifndef STATUSNOTIFIERTEST_H
#define STATUSNOTIFIERTEST_H

#include <QDialog>
#include <QWidget>

#include <KJob>

#include "ui_statusnotifiertest.h"

class StatusNotifierTestPrivate;

class StatusNotifierTest : public QDialog, public Ui_StatusNotifierTest
{
    Q_OBJECT

public:
    StatusNotifierTest(QWidget *parent = nullptr);
    ~StatusNotifierTest() override;

    void init();
    void log(const QString &msg);

public Q_SLOTS:
    int runMain();
    void timeout();
    void updateUi();
    void updateNotifier();

    void activateRequested(bool active, const QPoint &pos);
    void scrollRequested(int delta, Qt::Orientation orientation);
    void secondaryActivateRequested(const QPoint &pos);

    void enableJob(bool enable = true);
    void setJobProgress(KJob *j, unsigned long v);
    void result(KJob *job);

private:
    StatusNotifierTestPrivate *d;
};

#endif
