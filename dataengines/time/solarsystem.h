/*
    SPDX-FileCopyrightText: 2009 Petri Damsten <damu@iki.fi>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QPair>
#include <QPointF>

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

class SolarSystemObject
{
public:
    SolarSystemObject();
    virtual ~SolarSystemObject();

    double meanLongitude() const
    {
        return L;
    };
    double meanAnomaly() const
    {
        return M;
    };
    double siderealTime();
    double altitude() const
    {
        return m_altitude;
    };
    double azimuth() const
    {
        return m_azimuth;
    };
    double calcElevation();
    QDateTime dateTime() const
    {
        return m_local;
    };
    double lambda() const
    {
        return m_lambda;
    };
    double eclipticLongitude() const
    {
        return m_eclipticLongitude;
    };
    void setPosition(double latitude, double longitude);

    virtual void calcForDateTime(const QDateTime &local, int offset);
    QList<QPair<QDateTime, QDateTime>> timesForAngles(const QList<double> &angles, const QDateTime &dt, int offset);

protected:
    void calc();
    virtual bool calcPerturbations(double *, double *, double *)
    {
        return false;
    };
    virtual void rotate(double *, double *){};
    virtual void topocentricCorrection(double *, double *){};

    inline double rev(double x);
    inline double asind(double x);
    inline double sind(double x);
    inline double cosd(double x);
    inline double atand(double x);
    inline double tand(double x);
    inline double atan2d(double y, double x);
    void toRectangular(double lo, double la, double r, double *x, double *y, double *z);
    void toSpherical(double x, double y, double z, double *lo, double *la, double *r);
    QPair<double, double> zeroPoints(QPointF p1, QPointF p2, QPointF p3);

    double N;
    double i;
    double w;
    double a;
    double e;
    double M;
    double m_obliquity;

    QDateTime m_utc;
    QDateTime m_local;
    double m_day;
    double m_latitude;
    double m_longitude;

    double L;
    double rad;
    double RA;
    double dec;
    double HA;
    double m_altitude;
    double m_azimuth;
    double m_eclipticLongitude;
    double m_lambda;
};

class Sun : public SolarSystemObject
{
public:
    Sun();
    void calcForDateTime(const QDateTime &local, int offset) override;

protected:
    void rotate(double *, double *) override;
};

class Moon : public SolarSystemObject
{
public:
    explicit Moon(Sun *sun);
    ~Moon() override{}; // to not delete the Sun

    void calcForDateTime(const QDateTime &local, int offset) override;
    double phase();

protected:
    bool calcPerturbations(double *RA, double *dec, double *r) override;
    void rotate(double *, double *) override;
    void topocentricCorrection(double *, double *) override;

private:
    Sun *m_sun;
};
