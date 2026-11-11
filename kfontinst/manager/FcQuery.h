#pragma once

/*
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
        , m_proc(nullptr)
    {
    }
    ~CFcQuery() override;

    void run(const QString &query);

    const QString &font() const
    {
        return m_font;
    }
    const QString &file() const
    {
        return m_file;
    }

private Q_SLOTS:

    void procExited();
    void data();

Q_SIGNALS:

    void finished();

private:
    QProcess *m_proc;
    QByteArray m_buffer;
    QString m_file, m_font;
};

}
