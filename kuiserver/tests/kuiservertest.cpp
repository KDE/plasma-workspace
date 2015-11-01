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


#include <kio/jobclasses.h>
#include <kuiserverjobtracker.h>
#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>

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
    connect(timer, &QTimer::timeout, this,
            &KJobTest::timerTimeout);

    connect(clockTimer, &QTimer::timeout, this,
            &KJobTest::updateMessage);

    timer->setSingleShot(true);
    timer->start(seconds * 1000);

    updateMessage();

    clockTimer->start(1000);
}

void KJobTest::timerTimeout()
{
    clockTimer->stop();

    emitResult();

    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
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



int main(int argc, char **argv)
{
    KAboutData aboutData( QLatin1String("kjobtest"), i18n("KJobTest"), QLatin1String("0.01"));
    aboutData.setShortDescription(i18n("A KJob tester"));
    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);


    KJobTest *myJob = new KJobTest(10 /* 10 seconds before it gets removed */);
    myJob->setUiDelegate(new KIO::JobUiDelegate());
    KIO::getJobTracker()->registerJob(myJob);
    myJob->start();

    return app.exec();
}
