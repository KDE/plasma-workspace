/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpolkde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QUrl url(app.arguments().constLast());
    QString moduleName = url.path();
    if (!url.isRelative()) {
        moduleName = moduleName.mid(1);
    }
    int ret = 0;
    if (!QStandardPaths::findExecutable("systemsettings5").isEmpty()) {
        QProcess::startDetached("systemsettings5", {moduleName});
    } else if (!QStandardPaths::findExecutable("plasma-settings").isEmpty()) {
        QProcess::startDetached("plasma-settings", {"-m", moduleName});
    } else if (!QStandardPaths::findExecutable("kcmshell5").isEmpty()) {
        QProcess::startDetached("kcmshell5", {moduleName});
    } else {
        QTextStream err(stderr);
        err << "Could not open;" << moduleName << url.toString() << Qt::endl;
        ret = 1;
    }
    return ret;
}
