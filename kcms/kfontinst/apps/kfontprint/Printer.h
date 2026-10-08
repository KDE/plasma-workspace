/*
    SPDX-FileCopyrightText: 2011 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "Misc.h"
#include <QDialog>
#include <QFont>
#include <QList>
#include <QThread>

class QLabel;
class QProgressBar;
class QPrinter;

namespace KFI
{
class CActionLabel;

class CPrintThread : public QThread
{
    Q_OBJECT

public:
    CPrintThread(QPrinter *printer, const QList<Misc::TFont> &items, int size, QObject *parent);
    ~CPrintThread() override;

    void run() override;

Q_SIGNALS:

    void progress(int p, const QString &f);

public Q_SLOTS:

    void cancel();

private:
    QPrinter *m_printer;
    QList<Misc::TFont> m_items;
    int m_size;
    bool m_cancelled;
};

class CPrinter : public QDialog
{
    Q_OBJECT

public:
    explicit CPrinter(QWidget *parent);
    ~CPrinter() override;

    void print(const QList<Misc::TFont> &items, int size);

Q_SIGNALS:

    void cancelled();

public Q_SLOTS:

    void progress(int p, const QString &label);
    void slotCancelClicked();

private:
    void closeEvent(QCloseEvent *e) override;

private:
    QLabel *m_statusLabel;
    QProgressBar *m_progress;
    CActionLabel *m_actionLabel;
};

}
