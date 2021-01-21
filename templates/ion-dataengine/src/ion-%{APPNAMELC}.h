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

#ifndef ION % {APPNAMEUC } _H
#define ION % {APPNAMEUC} _H

#include <plasma/weather/ion.h>

class Q_DECL_EXPORT % {APPNAME} Ion : public IonInterface
{
    Q_OBJECT

public:
    % {APPNAME} Ion(QObject * parent, const QVariantList &args);
    ~ % {APPNAME} Ion();

public: // IonInterface API
    bool updateIonSource(const QString &source) override;
    void reset() override;

private:
    void fetchValidateData(const QString &source);
    void fetchWeatherData(const QString &source);

    void onValidateReport(const QString &source);
    void onWeatherDataReport(const QString &source);
};

#endif
