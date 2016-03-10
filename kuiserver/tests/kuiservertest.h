/**
  * This file is part of the KDE libraries
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef __KJOBTEST_H__
#define __KJOBTEST_H__

#include <kjob.h>
#include <kjobuidelegate.h>
#include <kio/jobclasses.h>

class QTimer;

class KJobTest
        : public KIO::Job
{
    Q_OBJECT

public:
    KJobTest(int numberOfSeconds = 5);
    ~KJobTest() override;

    void start() override;

private Q_SLOTS:
    void timerTimeout();
    void updateMessage();

protected:
    bool doSuspend() override;

private:
    QTimer *timer, *clockTimer;
    int seconds, total;
};

#endif // __KJOBTEST_H__
