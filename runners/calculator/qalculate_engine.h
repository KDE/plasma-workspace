/*
 *   Copyright 2010 Matteo Agostinelli <agostinelli@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef QALCULATEENGINE_H
#define QALCULATEENGINE_H

#include <QAtomicInt>
#include <QObject>

class KJob;

class QalculateEngine : public QObject
{
    Q_OBJECT
public:
    explicit QalculateEngine(QObject *parent = nullptr);
    ~QalculateEngine() override;

    QString lastResult() const
    {
        return m_lastResult;
    }

public Q_SLOTS:
    QString evaluate(const QString &expression, bool *isApproximate = nullptr);
    void updateExchangeRates();

    void copyToClipboard(bool flag = true);

protected Q_SLOTS:
    void updateResult(KJob *);

Q_SIGNALS:
    void resultReady(const QString &);
    void formattedResultReady(const QString &);

private:
    QString m_lastResult;
    static QAtomicInt s_counter;
};

#endif // QALCULATEENGINE_H
