/*
 *   Copyright (C) %{CURRENT_YEAR} by %{AUTHOR} <%{EMAIL}>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
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

#include "ion-%{APPNAMELC}.h"

#include <KUnitConversion/Converter>


%{APPNAME}Ion::%{APPNAME}Ion(QObject *parent, const QVariantList &args)
  : IonInterface(parent, args)
{
    // call whenever the ion is ready
    setInitialized(true);
}

%{APPNAME}Ion::~%{APPNAME}Ion()
{
}

bool %{APPNAME}Ion::updateIonSource(const QString& source)
{
    // We expect the applet to send the source in the following tokenization:
    // ionname:validate:place_name - Triggers validation of place
    // ionname:weather:place_name - Triggers receiving weather of place

    const QStringList sourceAction = source.split(QLatin1Char('|'));

    // Guard: if the size of array is not 2 then we have bad data, return an error
    if (sourceAction.size() < 2) {
        setData(source, QStringLiteral("validate"), "%{APPNAMELC}|malformed");
        return true;
    }

    if (sourceAction.at(1) == QLatin1String("validate") && sourceAction.size() > 2) {
        fetchValidateData(source);
        return true;
    } 

    if (sourceAction.at(1) == QLatin1String("weather") && sourceAction.size() > 2) {
        fetchWeatherData(source);
        return true;
    }

    setData(source, QStringLiteral("validate"), "%{APPNAMELC}|malformed");
    return true;
}

void %{APPNAME}Ion::reset()
{
}

// purpose: fetch/use data from provider and trigger processing of returned data in a handler
void %{APPNAME}Ion::fetchValidateData(const QString &source)
{

    // here called directly for a start
    onValidateReport(source);
}

// purpose: process data from provider and turn into DataEngine data
void %{APPNAME}Ion::onValidateReport(const QString &source)
{

    const QStringList sourceAction = source.split(QLatin1Char('|'));

    QStringList placeList;
    placeList.append(QStringLiteral("place|").append(QStringLiteral("Some example place")));

    if (placeList.size() == 1) {
        setData(source, QStringLiteral("validate"), QStringLiteral("%{APPNAMELC}|valid|single|").append(placeList.at(0)));
        return;
    }
    if (placeList.size() > 1) {
        setData(source, QStringLiteral("validate"), QStringLiteral("%{APPNAMELC}|valid|multiple|").append(placeList.join(QLatin1Char('|'))));
        return;
    }
    if (placeList.size() == 0) {
        setData(source, QStringLiteral("validate"), QStringLiteral("%{APPNAMELC}|invalid|single|").append(sourceAction.at(2)));
        return;
    }
}

// purpose: fetch data from provider and trigger processing of returned data in a handler
void %{APPNAME}Ion::fetchWeatherData(const QString &source)
{
    // here called directly for a start
    onWeatherDataReport(source);
}


// purpose: process data from provider and turn into DataEngine data for the given source key
void %{APPNAME}Ion::onWeatherDataReport(const QString &source)
{
    Plasma::DataEngine::Data data;

    // examples, see the existing ion dataengines for other keys in use:
    // plasma-workspace/dataengines/weather/ions/
    // For an overview of all common keys and value types see plasma-workspace/dataengines/weather/ions/ion.h
    data.insert(QStringLiteral("Place"), "Some %{APPNAME} place");
    data.insert(QStringLiteral("Station"), "Some %{APPNAME} station");
    data.insert(QStringLiteral("Credit"), "%{APPNAME} weather data provider");
    data.insert(QStringLiteral("Temperature"), 23.4);
    data.insert(QStringLiteral("Temperature Unit"), QString::number(KUnitConversion::Celsius));

    // finally set the created data for the given source key, so it will be pushed out to all consumers
    setData(source, data);
}


K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(%{APPNAMELC}, %{APPNAME}Ion, "ion-%{APPNAMELC}.json")

#include "ion-%{APPNAMELC}.moc"
