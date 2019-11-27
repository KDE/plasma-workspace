/*
 *   Copyright (C) 2009 Petri Damsten <damu@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "solarsystem.h"
#include <math.h>
#include <QList>

/*
 *   Mathematics, ideas, public domain code used for these classes from:
 *   https://www.stjarnhimlen.se/comp/tutorial.html
 *   https://www.stjarnhimlen.se/comp/riset.html
 *   https://www.esrl.noaa.gov/gmd/grad/solcalc/azel.html
 *   https://www.esrl.noaa.gov/gmd/grad/solcalc/sunrise.html
 *   http://web.archive.org/web/20080309162302/http://bodmas.org/astronomy/riset.html
 *   moontool.c by John Walker
 *   Wikipedia
 */

Sun::Sun()
    : SolarSystemObject()
{
}

void Sun::calcForDateTime(const QDateTime& local, int offset)
{
    SolarSystemObject::calcForDateTime(local, offset);

    N = 0.0;
    i = 0.0;
    w = rev(282.9404 + 4.70935E-5 * m_day);
    a = 1.0;
    e = rev(0.016709 - 1.151E-9 * m_day);
    M = rev(356.0470 + 0.9856002585 * m_day);

    calc();
}

void Sun::rotate(double* y, double* z)
{
    *y *= cosd(m_obliquity);
    *z *= sind(m_obliquity);
}

Moon::Moon(Sun *sun)
    : m_sun(sun)
{
}

void Moon::calcForDateTime(const QDateTime& local, int offset)
{
    if (m_sun->dateTime() != local) {
        m_sun->calcForDateTime(local, offset);
    }

    SolarSystemObject::calcForDateTime(local, offset);

    N = rev(125.1228 - 0.0529538083 * m_day);
    i = 5.1454;
    w = rev(318.0634 + 0.1643573223 * m_day);
    a = 60.2666;
    e = 0.054900;
    M = rev(115.3654 + 13.0649929509 * m_day);

    calc();
}

bool Moon::calcPerturbations(double *lo, double *la, double *r)
{
    double Ms = m_sun->meanAnomaly();
    double D = L - m_sun->meanLongitude();
    double F = L - N;

    *lo +=  -1.274 * sind(M - 2 * D)
            +0.658 * sind(2 * D)
            -0.186 * sind(Ms)
            -0.059 * sind(2 * M - 2 * D)
            -0.057 * sind(M - 2 * D + Ms)
            +0.053 * sind(M + 2 * D)
            +0.046 * sind(2 * D - Ms)
            +0.041 * sind(M - Ms)
            -0.035 * sind(D)
            -0.031 * sind(M + Ms)
            -0.015 * sind(2 * F - 2 * D)
            +0.011 * sind(M - 4 * D);
    *la +=  -0.173 * sind(F - 2 * D)
            -0.055 * sind(M - F - 2 * D)
            -0.046 * sind(M + F - 2 * D)
            +0.033 * sind(F + 2 * D)
            +0.017 * sind(2 * M + F);
    *r +=   -0.58 * cosd(M - 2 * D)
            -0.46 * cosd(2 * D);
    return true;
}

void Moon::topocentricCorrection(double* RA, double* dec)
{
    double HA = rev(siderealTime() - *RA);
    double gclat = m_latitude - 0.1924 * sind(2 * m_latitude);
    double rho = 0.99833 + 0.00167 * cosd(2 * m_latitude);
    double mpar = asind(1 / rad);
    double g = atand(tand(gclat) / cosd(HA));

    *RA -= mpar * rho * cosd(gclat) * sind(HA) / cosd(*dec);
    *dec -= mpar * rho * sind(gclat) * sind(g - *dec) / sind(g);
}

double Moon::phase()
{
   return rev(m_eclipticLongitude - m_sun->lambda());
}

void Moon::rotate(double* y, double* z)
{
    double t = *y;
    *y = t * cosd(m_obliquity) - *z * sind(m_obliquity);
    *z = t * sind(m_obliquity) + *z * cosd(m_obliquity);
}

void SolarSystemObject::calc()
{
    double x, y, z;
    double la, r;

    L = rev(N + w + M);
    double E0 = 720.0;
    double E = M + (180.0 / M_PI) * e * sind(M) * (1.0 + e * cosd(M));
    for (int j = 0; fabs(E0 - E) > 0.005 && j < 10; ++j) {
        E0 = E;
        E = E0 - (E0 - (180.0 / M_PI) * e * sind(E0) - M) / (1 - e * cosd(E0));
    }
    x = a * (cosd(E) - e);
    y = a * sind(E) * sqrt(1.0 - e * e);
    r = sqrt(x * x + y * y);
    double v = rev(atan2d(y, x));
    m_lambda = rev(v + w);
    x = r * (cosd(N) * cosd(m_lambda) - sind(N) * sind(m_lambda) * cosd(i));
    y = r * (sind(N) * cosd(m_lambda) + cosd(N) * sind(m_lambda) * cosd(i));
    z = r * sind(m_lambda);
    if (!qFuzzyCompare(i, 0.0)) {
        z *= sind(i);
    }
    toSpherical(x, y, z, &m_eclipticLongitude, &la, &r);
    if (calcPerturbations(&m_eclipticLongitude, &la, &r)) {
        toRectangular(m_eclipticLongitude, la, r, &x, &y, &z);
    }
    rotate(&y, &z);
    toSpherical(x, y, z, &RA, &dec, &rad);
    topocentricCorrection(&RA, &dec);

    HA = rev(siderealTime() - RA);
    x = cosd(HA) * cosd(dec) * sind(m_latitude) - sind(dec) * cosd(m_latitude);
    y = sind(HA) * cosd(dec);
    z = cosd(HA) * cosd(dec) * cosd(m_latitude) + sind(dec) * sind(m_latitude);
    m_azimuth = atan2d(y, x) + 180.0;
    m_altitude = asind(z);
}

double SolarSystemObject::siderealTime()
{
    double UT = m_utc.time().hour() + m_utc.time().minute() / 60.0 +
                m_utc.time().second() / 3600.0;
    double GMST0 = rev(282.9404 + 4.70935E-5 * m_day + 356.0470 + 0.9856002585 * m_day + 180.0);
    return GMST0 + UT * 15.0 + m_longitude;
}

void SolarSystemObject::calcForDateTime(const QDateTime& local, int offset)
{
    m_local = local;
    m_utc = local.addSecs(-offset);
    m_day = 367 * m_utc.date().year() - (7 * (m_utc.date().year() +
            ((m_utc.date().month() + 9) / 12))) / 4 +
            (275 * m_utc.date().month()) / 9 + m_utc.date().day() - 730530;
    m_day += m_utc.time().hour() / 24.0 + m_utc.time().minute() / (24.0 * 60.0) +
             m_utc.time().second() / (24.0 * 60.0 * 60.0);
    m_obliquity = 23.4393 - 3.563E-7 * m_day;
}

SolarSystemObject::SolarSystemObject()
    : m_latitude(0.0)
    , m_longitude(0.0)
{
}

SolarSystemObject::~SolarSystemObject()
{
}

void SolarSystemObject::setPosition(double latitude, double longitude)
{
    m_latitude = latitude;
    m_longitude = longitude;
}

double SolarSystemObject::rev(double x)
{
    return  x - floor(x / 360.0) * 360.0;
}

double SolarSystemObject::asind(double x)
{
    return asin(x) * 180.0 / M_PI;
}

double SolarSystemObject::sind(double x)
{
    return sin(x * M_PI / 180.0);
}

double SolarSystemObject::cosd(double x)
{
    return cos(x * M_PI / 180.0);
}

double SolarSystemObject::tand(double x)
{
    return tan(x * M_PI / 180.0);
}

double SolarSystemObject::atan2d(double y, double x)
{
    return atan2(y, x) * 180.0 / M_PI;
}

double SolarSystemObject::atand(double x)
{
    return atan(x) * 180.0 / M_PI;
}

void SolarSystemObject::toRectangular(double lo, double la, double r, double *x, double *y, double *z)
{
    *x = r * cosd(lo) * cosd(la);
    *y = r * sind(lo) * cosd(la);
    *z = r * sind(la);
}

void SolarSystemObject::toSpherical(double x, double y, double z, double *lo, double *la, double *r)
{
    *r = sqrt(x * x + y * y + z * z);
    *la = asind(z / *r);
    *lo = rev(atan2d(y, x));
}

QPair<double, double> SolarSystemObject::zeroPoints(QPointF p1, QPointF p2, QPointF p3)
{
    double a = ((p2.y() - p1.y()) * (p1.x() - p3.x()) + (p3.y() - p1.y()) * (p2.x() - p1.x())) /
               ((p1.x() - p3.x()) * (p2.x() * p2.x() - p1.x() * p1.x()) + (p2.x() - p1.x()) *
                (p3.x() * p3.x() - p1.x() * p1.x()));
    double b = ((p2.y() - p1.y()) - a * (p2.x() * p2.x() - p1.x() * p1.x())) / (p2.x() - p1.x());
    double c = p1.y() - a * p1.x() * p1.x() - b * p1.x();
    double discriminant = b * b - 4.0 * a * c;
    double z1 = -1.0, z2 = -1.0;

    if (discriminant >= 0.0) {
        z1 = (-b + sqrt(discriminant)) / (2 * a);
        z2 = (-b - sqrt(discriminant)) / (2 * a);
    }
    return QPair<double, double>(z1, z2);
}

QList< QPair<QDateTime, QDateTime> > SolarSystemObject::timesForAngles(const QList<double>& angles,
                                                                       const QDateTime& dt,
                                                                       int offset)
{
    QList<double> altitudes;
    QDate d = dt.date();
    QDateTime local(d, QTime(0, 0));
    for (int j = 0; j <= 25; ++j) {
        calcForDateTime(local, offset);
        altitudes.append(altitude());
        local = local.addSecs(60 * 60);
    }
    QList< QPair<QDateTime, QDateTime> > result;
    QTime rise, set;
    foreach (double angle, angles) {
        for (int j = 3; j <= 25; j += 2) {
            QPointF p1((j - 2) * 60 * 60, altitudes[j - 2] - angle);
            QPointF p2((j - 1) * 60 * 60, altitudes[j - 1] - angle);
            QPointF p3(j * 60 * 60, altitudes[j] - angle);
            QPair<double, double> z = zeroPoints(p1, p2, p3);
            if (z.first > p1.x() && z.first < p3.x()) {
                if (p1.y() < 0.0) {
                    rise = QTime(0, 0).addSecs(z.first);
                } else {
                    set = QTime(0, 0).addSecs(z.first);
                }
            }
            if (z.second > p1.x() && z.second < p3.x()) {
                if (p3.y() < 0.0) {
                    set = QTime(0, 0).addSecs(z.second);
                } else {
                    rise = QTime(0, 0).addSecs(z.second);
                }
            }
        }
        result.append(QPair<QDateTime, QDateTime>(QDateTime(d, rise), QDateTime(d, set)));
    }
    return result;
}

double SolarSystemObject::calcElevation()
{
    double refractionCorrection;

    if (m_altitude > 85.0) {
        refractionCorrection = 0.0;
    } else {
        double te = tand(m_altitude);
        if (m_altitude > 5.0) {
            refractionCorrection = 58.1 / te - 0.07 / (te * te * te) +
                                   0.000086 / (te * te * te * te * te);
        } else if (m_altitude > -0.575) {
            refractionCorrection = 1735.0 + m_altitude *
                (-518.2 + m_altitude * (103.4 + m_altitude *
                (-12.79 + m_altitude * 0.711) ) );
        } else {
            refractionCorrection = -20.774 / te;
        }
        refractionCorrection = refractionCorrection / 3600.0;
    }
    return m_altitude + refractionCorrection;
}
