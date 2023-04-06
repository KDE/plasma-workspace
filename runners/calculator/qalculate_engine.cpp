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
        qDebug() << "The exchange rates could not be updated. The following error has been reported:" << job->errorString();
    } else {
        // the exchange rates have been successfully updated, now load them
        CALCULATOR->loadExchangeRates();
    }
}

#if QALCULATE_MAJOR_VERSION > 2 || QALCULATE_MINOR_VERSION > 6
bool has_error()
{
    while (CALCULATOR->message()) {
        if (CALCULATOR->message()->type() == MESSAGE_ERROR) {
            CALCULATOR->clearMessages();
            return true;
        }
        CALCULATOR->nextMessage();
    }
    return false;
}

bool check_valid_before(const std::string &expression, const EvaluationOptions &search_eo)
{
    bool b_valid = false;
    if (!b_valid)
        b_valid = (expression.find_first_of(OPERATORS NUMBERS PARENTHESISS) != std::string::npos);
    if (!b_valid)
        b_valid = CALCULATOR->hasToExpression(expression, false, search_eo);
    if (!b_valid) {
        std::string str = expression;
        CALCULATOR->parseSigns(str);
        b_valid = (str.find_first_of(OPERATORS NUMBERS PARENTHESISS) != std::string::npos);
        if (!b_valid) {
            size_t i = str.find_first_of(SPACES);
            MathStructure m;
            if (!b_valid) {
                CALCULATOR->parse(&m, str, search_eo.parse_options);
                if (!has_error() && (m.isUnit() || m.isFunction() || (m.isVariable() && (i != std::string::npos || m.variable()->isKnown()))))
                    b_valid = true;
            }
        }
    }
    return b_valid;
}
#endif

QString QalculateEngine::evaluate(const QString &expression, bool *isApproximate, int base, const QString &customBase)
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

#if QALCULATE_MAJOR_VERSION > 2 || QALCULATE_MINOR_VERSION > 6
    if (!check_valid_before(expression.toStdString(), eo)) {
        return QString(); // See https://github.com/Qalculate/libqalculate/issues/442
    }
#endif

    CALCULATOR->setPrecision(16);
#ifdef BASE_CUSTOM // v3.3.0 has setCustomOutputBase
    if (base == BASE_CUSTOM) {
        EvaluationOptions eo;
        eo.parse_options.base = 10;
        eo.approximation = APPROXIMATION_TRY_EXACT;

        MathStructure m = CALCULATOR->calculate(customBase.toStdString(), eo);

        if (m.isNumber() && (m.number().isPositive() || m.number().isInteger()) && (m.number().isGreaterThan(1) || m.number().isLessThan(-1))) {
            CALCULATOR->setCustomOutputBase(m.number());
        } else {
            base = BASE_DECIMAL;
        }
    }
#endif

    constexpr int timeout = 10000;
    MathStructure result;
    if (!CALCULATOR->calculate(&result, ctext, timeout, eo)) {
        // BUG 468084: stop libqalculate thread if timeout is reached
        return {};
    }

    PrintOptions po;
    po.base = base;
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

    m_lastResult = QString::fromStdString(CALCULATOR->print(result, timeout, po));

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

static const QMap<QString, int> s_commonBaseMappings = {
    {QStringLiteral("roman"), BASE_ROMAN_NUMERALS},
    {QStringLiteral("time"), BASE_TIME},

    {QStringLiteral("bin"), BASE_BINARY},
    {QStringLiteral("oct"), BASE_OCTAL},
    {QStringLiteral("dec"), BASE_DECIMAL},
    {QStringLiteral("duo"), 12},
    {QStringLiteral("hex"), BASE_HEXADECIMAL},
    {QStringLiteral("sexa"), BASE_SEXAGESIMAL},

#ifdef BASE_CUSTOM // v3.3.0
    {QStringLiteral("pi"), BASE_PI},
    {QStringLiteral("e"), BASE_E},
    {QStringLiteral("sqrt2"), BASE_SQRT2},

    {QStringLiteral("golden"), BASE_GOLDEN_RATIO},
    {QStringLiteral("supergolden"), BASE_SUPER_GOLDEN_RATIO},

    {QStringLiteral("unicode"), BASE_UNICODE},
#endif // v3.3.0

#ifdef BASE_BIJECTIVE_26 // v3.5.0
    {QStringLiteral("bijective"), BASE_BIJECTIVE_26},
    {QStringLiteral("b26"), BASE_BIJECTIVE_26},
#endif // v3.5.0

#ifdef BASE_FP16 // v3.8.0
    {QStringLiteral("fp16"), BASE_FP16},
    {QStringLiteral("fp32"), BASE_FP32},
    {QStringLiteral("fp64"), BASE_FP64},
    {QStringLiteral("fp80"), BASE_FP80},
    {QStringLiteral("fp128"), BASE_FP128},
#endif // v3.8.0

#ifdef BASE_SEXAGESIMAL_2 // v3.18.0
    {QStringLiteral("sexa2"), BASE_SEXAGESIMAL_2},
    {QStringLiteral("sexa3"), BASE_SEXAGESIMAL_3},

    {QStringLiteral("latitude"), BASE_LATITUDE},
    {QStringLiteral("latitude2"), BASE_LATITUDE_2},
    {QStringLiteral("longitude"), BASE_LONGITUDE},
    {QStringLiteral("longitude2"), BASE_LONGITUDE_2},
#endif

#ifdef BASE_BINARY_DECIMAL // v4.2.0
    {QStringLiteral("bcd"), BASE_BINARY_DECIMAL},
#endif
};

bool QalculateEngine::findPrefix(QString basePrefix, int *base, QString *customBase)
{
    if (basePrefix.isEmpty()) {
        return true;
    }

    basePrefix = basePrefix.toLower();
    if (s_commonBaseMappings.contains(basePrefix)) {
        *base = s_commonBaseMappings[basePrefix];
        return true;
    }
#ifdef BASE_CUSTOM // v3.3.0
    if (basePrefix.startsWith("base")) {
        *base = BASE_CUSTOM;
        *customBase = basePrefix.mid(4);

        return true;
    }
#endif
    return false;
}
