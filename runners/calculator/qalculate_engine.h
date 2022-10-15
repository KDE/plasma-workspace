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

    static bool findPrefix(QString basePrefix, int *base, QString *customBase);

public Q_SLOTS:
    QString evaluate(const QString &expression, bool *isApproximate = nullptr, int base = 10, const QString &customBase = "");
    void updateExchangeRates();

    void copyToClipboard(bool flag = true);

protected Q_SLOTS:
    void updateResult(KJob *);

private:
    QString m_lastResult;
    static QAtomicInt s_counter;
};
