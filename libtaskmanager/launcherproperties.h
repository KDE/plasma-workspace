/*****************************************************************

Copyright (C) 2011 Craig Drummond <craig@kde.org>

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

#ifndef LAUNCHER_PROPERTIES_H
#define LAUNCHER_PROPERTIES_H

#include <QDialog>
#include "ui_launcherproperties.h"

class QDialogButtonBox;
namespace TaskManager
{

class LauncherProperties : public QDialog
{
    Q_OBJECT

public:
    LauncherProperties(QWidget *parent = 0L);
    virtual ~LauncherProperties();

    void run(const QString &cc = QString(), const QString &cn = QString(), const QString &l = QString());

Q_SIGNALS:
    void properties(const QString &, const QString &, const QString &);

private Q_SLOTS:
    void check();
    void detect();
    void browse();
    void launcherSelected();
    void okClicked();

private:
    bool eventFilter(QObject *o, QEvent *e);
    WId findWindow();

private:
    Ui::LauncherProperties ui;
    QDialog                *grabber;
    QDialogButtonBox       *buttons;
};

}

#endif
