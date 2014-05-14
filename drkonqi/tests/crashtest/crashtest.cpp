/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2002 David Faure <faure@kde.org>
 * Copyright (C) 2000-2002 Waldo Bastian <bastian@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

// Let's crash.
#include <kcrash.h>
#include <kaboutdata.h>
#include <assert.h>
#include <QtConcurrentMap>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QApplication>
#include <KAboutData>

enum CrashType { Crash, Malloc, Div0, Assert, QAssert, Threads };

struct SomeStruct
{
    int foo() { return ret; }
    int ret;
};

void do_crash()
{
  SomeStruct *obj = 0;
  int ret = obj->foo();
  printf("result = %d\n", ret);
}

void do_malloc()
{
  delete (char*)0xdead;
}

void do_div0()
{
  volatile int a = 99;
  volatile int b = 10;
  volatile int c = a / ( b - 10 );
  printf("result = %d\n", c);
}

void do_assert()
{
  assert(false);
}

void do_qassert()
{
  Q_ASSERT(false);
}

void map_function(const QString & s)
{
    while ( s != "thread 4" ) {}
    do_crash();
}

void do_threads()
{
    QStringList foo;
    foo << "thread 1" << "thread 2" << "thread 3" << "thread 4" << "thread 5";
    QThreadPool::globalInstance()->setMaxThreadCount(5);
    QtConcurrent::blockingMap(foo, map_function);
}

void level4(int t)
{
  if (t == Malloc)
    do_malloc();
  else if (t == Div0)
    do_div0();
  else if (t == Assert)
    do_assert();
  else if (t == QAssert)
    do_qassert();
  else if (t == Threads)
    do_threads();
  else
    do_crash();
}

void level3(int t)
{
  level4(t);
}

void level2(int t)
{
  level3(t);
}

void level1(int t)
{
  level2(t);
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  KAboutData aboutData("crashtext", i18n("Crash Test for DrKonqi"),
                       "1.1",
                       i18n("Crash Test for DrKonqi"),
                       KAboutLicense::GPL,
                       i18n("(c) 2000-2002 David Faure, Waldo Bastian"));

  QCommandLineParser parser;
  parser.addOption(QCommandLineOption("autorestart", i18n("Automatically restart")));
  parser.addPositionalArgument("type", i18n("Type of crash."), "crash|malloc|div0|assert|threads");
  aboutData.setupCommandLine(&parser);
  parser.process(app);
  aboutData.processCommandLine(&parser);

  //start drkonqi directly so that drkonqi's output goes to the console
  KCrash::CrashFlags flags = KCrash::AlwaysDirectly;
  if (parser.isSet("autorestart"))
    flags |= KCrash::AutoRestart;
  KCrash::setFlags(flags);

  QByteArray type = parser.positionalArguments().isEmpty() ? QByteArray() : parser.positionalArguments().first().toUtf8();
  int crashtype = Crash;
  if (type == "malloc")
    crashtype = Malloc;
  else if (type == "div0")
    crashtype = Div0;
  else if (type == "assert")
    crashtype = Assert;
  else if (type == "qassert")
    crashtype = QAssert;
  else if (type == "threads")
    crashtype = Threads;
  level1(crashtype);
  return app.exec();
}
