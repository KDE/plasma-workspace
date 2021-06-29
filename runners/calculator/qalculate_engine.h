/*
    SPDX-FileCopyrightText: 2010 Matteo Agostinelli <agostinelli@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
