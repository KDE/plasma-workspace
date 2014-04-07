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

#include "kuiservertest.h"
#include <kio/jobuidelegate.h>
#include <QTimer>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kio/jobclasses.h>
#include <kuiserverjobtracker.h>

KJobTest::KJobTest(int numberOfSeconds)
        : KIO::Job(), timer(new QTimer(this)), clockTimer(new QTimer(this)),
        seconds(numberOfSeconds), total(numberOfSeconds)
{
    setCapabilities(KJob::NoCapabilities);
}

KJobTest::~KJobTest()
{
}

void KJobTest::start()
{
    connect(timer, SIGNAL(timeout()), this,
            SLOT(timerTimeout()));

    connect(clockTimer, SIGNAL(timeout()), this,
            SLOT(updateMessage()));

    timer->setSingleShot(true);
    timer->start(seconds * 1000);

    updateMessage();

    clockTimer->start(1000);
}

void KJobTest::timerTimeout()
{
    clockTimer->stop();

    emitResult();

    QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
}

void KJobTest::updateMessage()
{
    emit infoMessage(this, i18n("Testing kuiserver (%1 seconds remaining)", seconds), i18n("Testing kuiserver (%1 seconds remaining)", seconds));
    emitPercent(total - seconds, total);

    seconds--;
}

bool KJobTest::doSuspend()
{
    clockTimer->stop();

    Job::doSuspend();

    return true;
}

#include "kuiservertest.moc"

int main(int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, "kjobtest", 0, ki18n("KJobTest"), "0.01", ki18n("A KJob tester"));

    KApplication app;

    KJobTest *myJob = new KJobTest(10 /* 10 seconds before it gets removed */);
    myJob->setUiDelegate(new KIO::JobUiDelegate());
    KIO::getJobTracker()->registerJob(myJob);
    myJob->start();

    return app.exec();
}
