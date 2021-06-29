/*
    SPDX-FileCopyrightText: 2010 Matteo Agostinelli <agostinelli@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qalculate_engine.h"

#include <libqalculate/Calculator.h>
#include <libqalculate/ExpressionItem.h>
#include <libqalculate/Function.h>
#include <libqalculate/Prefix.h>
#include <libqalculate/Unit.h>
#include <libqalculate/Variable.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFile>

#include <KIO/Job>
#include <KLocalizedString>
#include <KProtocolManager>

QAtomicInt QalculateEngine::s_counter;

QalculateEngine::QalculateEngine(QObject *parent)
    : QObject(parent)
{
    s_counter.ref();
    if (!CALCULATOR) {
        new Calculator();
        CALCULATOR->terminateThreads();
        CALCULATOR->loadGlobalDefinitions();
        CALCULATOR->loadLocalDefinitions();
        CALCULATOR->loadGlobalCurrencies();
        CALCULATOR->loadExchangeRates();
    }
}

QalculateEngine::~QalculateEngine()
{
    if (s_counter.deref()) {
        delete CALCULATOR;
        CALCULATOR = nullptr;
    }
}

void QalculateEngine::updateExchangeRates()
{
    QUrl source = QUrl("http://www.ecb.int/stats/eurofxref/eurofxref-daily.xml");
    QUrl dest = QUrl::fromLocalFile(QFile::decodeName(CALCULATOR->getExchangeRatesFileName().c_str()));

    KIO::Job *getJob = KIO::file_copy(source, dest, -1, KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, &KJob::result, this, &QalculateEngine::updateResult);
}

void QalculateEngine::updateResult(KJob *job)
{
    if (job->error()) {
        qDebug() << i18n("The exchange rates could not be updated. The following error has been reported: %1", job->errorString());
    } else {
        // the exchange rates have been successfully updated, now load them
        CALCULATOR->loadExchangeRates();
    }
}

QString QalculateEngine::evaluate(const QString &expression, bool *isApproximate)
{
    if (expression.isEmpty()) {
        return QString();
    }

    QString input = expression;
    // Make sure to use toLocal8Bit, the expression can contain non-latin1 characters
    QByteArray ba = input.replace(QChar(0xA3), "GBP").replace(QChar(0xA5), "JPY").replace('$', "USD").replace(QChar(0x20AC), "EUR").toLocal8Bit();
    const char *ctext = ba.data();

    CALCULATOR->terminateThreads();
    EvaluationOptions eo;

    eo.auto_post_conversion = POST_CONVERSION_BEST;
    eo.keep_zero_units = false;

    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.structuring = STRUCTURING_SIMPLIFY;

    // suggested in https://github.com/Qalculate/libqalculate/issues/16
    // to avoid memory overflow for seemingly innocent calculations (Bug 277011)
    eo.approximation = APPROXIMATION_APPROXIMATE;

    CALCULATOR->setPrecision(16);
    MathStructure result = CALCULATOR->calculate(ctext, eo);

    PrintOptions po;
    po.number_fraction_format = FRACTION_DECIMAL;
    po.indicate_infinite_series = false;
    po.use_all_prefixes = false;
    po.use_denominator_prefix = true;
    po.negative_exponents = false;
    po.lower_case_e = true;
    po.base_display = BASE_DISPLAY_NORMAL;
#if defined(QALCULATE_MAJOR_VERSION) && defined(QALCULATE_MINOR_VERSION)                                                                                       \
    && (QALCULATE_MAJOR_VERSION > 2 || (QALCULATE_MAJOR_VERSION == 2 && QALCULATE_MINOR_VERSION >= 2))
    po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
#endif

    result.format(po);

    m_lastResult = result.print(po).c_str();

    if (isApproximate) {
        *isApproximate = result.isApproximate();
    }

    return m_lastResult;
}

void QalculateEngine::copyToClipboard(bool flag)
{
    Q_UNUSED(flag);

    QApplication::clipboard()->setText(m_lastResult);
}
