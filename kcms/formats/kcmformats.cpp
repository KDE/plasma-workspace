/*
    kcmformats.cpp
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QCollator>
#include <QtGlobal>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include "formatssettings.h"
#include "kcmformats.h"
#include "localelistmodel.h"
K_PLUGIN_CLASS_WITH_JSON(KCMFormats, "metadata.json")

KCMFormats::KCMFormats(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_settings(new FormatsSettings(this))
{
    KAboutData *aboutData = new KAboutData(QStringLiteral("kcm_formats"),
                                           i18nc("@title", "formats"),
                                           QStringLiteral("0.1"),
                                           QLatin1String(""),
                                           KAboutLicense::LicenseKey::GPL_V2,
                                           i18nc("@info:credit", "Copyright Year Author"));

    aboutData->addAuthor(i18nc("@info:credit", "Author"), i18nc("@info:credit", "Author"), QStringLiteral("author@domain.com"));

    setAboutData(aboutData);
    setButtons(Help | Apply | Default);
    setQuickHelp(
        i18n("<h1>Formats</h1>"
             "You can configure the formats used for time, dates, money and other numbers here."));

    qmlRegisterAnonymousType<FormatsSettings>("kcmformats", 1);
    qmlRegisterType<LocaleListModel>("LocaleListModel", 1, 0, "LocaleListModel");
    connect(m_settings, &FormatsSettings::langChanged, this, &KCMFormats::handleLangChange);
    connect(m_settings, &FormatsSettings::numericChanged, this, &KCMFormats::numericExampleChanged);
    connect(m_settings, &FormatsSettings::timeChanged, this, &KCMFormats::timeExampleChanged);
    connect(m_settings, &FormatsSettings::monetaryChanged, this, &KCMFormats::monetaryExampleChanged);
    connect(m_settings, &FormatsSettings::measurementChanged, this, &KCMFormats::measurementExampleChanged);
    connect(m_settings, &FormatsSettings::collateChanged, this, &KCMFormats::collateExampleChanged);
}

void KCMFormats::handleLangChange()
{
    auto defaultVal = i18n("Default");
    if (m_settings->numeric() == defaultVal)
        Q_EMIT numericExampleChanged();
    if (m_settings->time() == defaultVal)
        Q_EMIT timeExampleChanged();
    if (m_settings->measurement() == defaultVal)
        Q_EMIT measurementExampleChanged();
    if (m_settings->monetary() == defaultVal)
        Q_EMIT monetaryExampleChanged();
    if (m_settings->collate() == defaultVal)
        Q_EMIT collateExampleChanged();
}

QString KCMFormats::numberExample() const
{
    return QLocale(m_settings->numeric()).toString(1000.01);
}
QString KCMFormats::timeExample() const
{
    auto tloc = QLocale(m_settings->time());
    return i18n("%1 (long format)", tloc.toString(QDateTime::currentDateTime())) + QLatin1Char('\n')
        + i18n("%1 (short format)", tloc.toString(QDateTime::currentDateTime(), QLocale::ShortFormat));
}
QString KCMFormats::currencyExample() const
{
    return QLocale(m_settings->monetary()).toCurrencyString(24.00);
}
QString KCMFormats::measurementExample() const
{
    auto mloc = QLocale(m_settings->measurement());
    QString measurementExample;
    if (mloc.measurementSystem() == QLocale::ImperialUKSystem) {
        measurementExample = i18nc("Measurement combobox", "Imperial UK");
    } else if (mloc.measurementSystem() == QLocale::ImperialUSSystem || mloc.measurementSystem() == QLocale::ImperialSystem) {
        measurementExample = i18nc("Measurement combobox", "Imperial US");
    } else {
        measurementExample = i18nc("Measurement combobox", "Metric");
    }
    return measurementExample;
}
QString KCMFormats::collateExample() const
{
    auto example{QStringLiteral("abcdefgxyzABCDEFGXYZÅåÄäÖöÅåÆæØø")};
    auto collator{QCollator{m_settings->collate()}};
    std::sort(example.begin(), example.end(), collator);
    return example;
}
FormatsSettings *KCMFormats::settings() const
{
    return m_settings;
}

QQuickItem *KCMFormats::getSubPage(int index) const
{
    return subPage(index);
}
#include "kcmformats.moc"
