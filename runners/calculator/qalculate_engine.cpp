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

#include "qalculate_engine.h"

#include <libqalculate/Calculator.h>
#include <libqalculate/ExpressionItem.h>
#include <libqalculate/Unit.h>
#include <libqalculate/Prefix.h>
#include <libqalculate/Variable.h>
#include <libqalculate/Function.h>

#include <QFile>
#include <QApplication>
#include <QClipboard>
#include <QDebug>

#include <KLocalizedString>
#include <KProtocolManager>
#include <KIO/Job>

QAtomicInt QalculateEngine::s_counter;

QalculateEngine::QalculateEngine(QObject* parent):
    QObject(parent)
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

    KIO::Job* getJob = KIO::file_copy(source, dest, -1, KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, &KJob::result, this, &QalculateEngine::updateResult);
}

void QalculateEngine::updateResult(KJob* job)
{
    if (job->error()) {
        qDebug() << i18n("The exchange rates could not be updated. The following error has been reported: %1",job->errorString());
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
    QByteArray ba = input.replace(QChar(0xA3), "GBP").replace(QChar(0xA5), "JPY").replace('$', "USD").replace(QChar(0x20AC), "EUR").toLatin1();
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
    #if defined(QALCULATE_MAJOR_VERSION) && defined(QALCULATE_MINOR_VERSION) && (QALCULATE_MAJOR_VERSION > 2 || (QALCULATE_MAJOR_VERSION == 2 && QALCULATE_MINOR_VERSION >= 2))
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

