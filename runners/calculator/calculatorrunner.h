/*
    SPDX-FileCopyrightText: 2007 Barış Metin <baris@pardus.org.tr>
    SPDX-FileCopyrightText: 2010 Matteo Agostinelli <agostinelli@gmail.com>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QAction>
#include <QMimeData>

#ifdef ENABLE_QALCULATE
class QalculateEngine;
#endif

#include <krunner/abstractrunner.h>

/**
 * This class evaluates the basic expressions given in the interface.
 */
class CalculatorRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    CalculatorRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~CalculatorRunner() override;

    void match(Plasma::RunnerContext &context) override;

protected Q_SLOTS:
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;
    QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match) override;

private:
    QString calculate(const QString &term, bool *isApproximate);
    void userFriendlyMultiplication(QString &cmd);
    void userFriendlySubstitutions(QString &cmd);
#ifndef ENABLE_QALCULATE
    void powSubstitutions(QString &cmd);
    void hexSubstitutions(QString &cmd);
#endif

#ifdef ENABLE_QALCULATE
    QalculateEngine *m_engine;
#endif
    QList<QAction *> m_actions;
};
