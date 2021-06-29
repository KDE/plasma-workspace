#ifndef __FC_QUERY_H__
#define __FC_QUERY_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Misc.h"
#include <QByteArray>
#include <QObject>

class QProcess;

namespace KFI
{
class CFcQuery : public QObject
{
    Q_OBJECT

public:
    CFcQuery(QObject *parent)
        : QObject(parent)
        , itsProc(nullptr)
    {
    }
    ~CFcQuery() override;

    void run(const QString &query);

    const QString &font() const
    {
        return itsFont;
    }
    const QString &file() const
    {
        return itsFile;
    }

private Q_SLOTS:

    void procExited();
    void data();

Q_SIGNALS:

    void finished();

private:
    QProcess *itsProc;
    QByteArray itsBuffer;
    QString itsFile, itsFont;
};

}

#endif
