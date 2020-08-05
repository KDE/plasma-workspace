/*****************************************************************
ksmserver - the KDE session management server

Copyright 2016 Martin Graesslin <mgraesslin@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QtConcurrentRun>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QProcess>

#include <unistd.h>

#include <config.h>

static void readFromPipe(int pipe)
{
    QFile readPipe;
    if (!readPipe.open(pipe, QIODevice::ReadOnly)) {
        QCoreApplication::exit(1);
        return;
    }
    QByteArray result = readPipe.readLine();
    qDebug() << "!!!! Result from helper process: " << result;
    if (result.isEmpty()) {
        qDebug() << "!!!! Error";
        QCoreApplication::exit(1);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption shutdownAllowedOption(QStringLiteral("shutdown-allowed"),
                                             QStringLiteral("Whether the user is allowed to shut down the system."));
    parser.addOption(shutdownAllowedOption);

    QCommandLineOption chooseOption(QStringLiteral("choose"),
                                    QStringLiteral("Whether the user is offered the choices between logout, shutdown, etc."));
    parser.addOption(chooseOption);

    QCommandLineOption modeOption(QStringLiteral("mode"),
                                  QStringLiteral("The initial exit mode to offer to the user."),
                                  QStringLiteral("logout|shutdown|reboot"),
                                  QStringLiteral("logout"));
    parser.addOption(modeOption);

    parser.process(app);

    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
        return 1;
    }

    QProcess p;
    p.setProgram(QStringLiteral(LOGOUT_GREETER_BIN));
    QStringList arguments;
    if (parser.isSet(shutdownAllowedOption)) {
        arguments << QStringLiteral("--shutdown-allowed");
    }
    if (parser.isSet(chooseOption)) {
        arguments << QStringLiteral("--choose");
    }
    if (parser.isSet(modeOption)) {
        arguments << QStringLiteral("--mode");
        arguments << parser.value(modeOption);
    }
    arguments << QStringLiteral("--mode-fd");
    arguments << QString::number(pipeFds[1]);
    p.setArguments(arguments);

    QObject::connect(&p, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::errorOccurred), &app,
        [] {
            QCoreApplication::exit(1);
        }
    );

    const int resultPipe = pipeFds[0];
    QObject::connect(&p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), &app,
        [resultPipe] (int exitCode) {
            if (exitCode != 0) {
                qDebug() << "!!!! finished with exit code: " << exitCode;
                close(resultPipe);
                QCoreApplication::exit(1);
                return;
            }
            QFutureWatcher<void> *watcher = new QFutureWatcher<void>();
            QObject::connect(watcher, &QFutureWatcher<void>::finished, QCoreApplication::instance(), &QCoreApplication::quit, Qt::QueuedConnection);
            QObject::connect(watcher, &QFutureWatcher<void>::finished, watcher, &QFutureWatcher<void>::deleteLater, Qt::QueuedConnection);
            watcher->setFuture(QtConcurrent::run(readFromPipe, resultPipe));
        }
    );

    p.start();
    close(pipeFds[1]);

    return app.exec();
}
