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

#ifndef SOLARSYSTEM_H
#define SOLARSYSTEM_H

#include <QDateTime>
#include <QPair>
#include <QPointF>

/*
 *   Mathematics, ideas, public domain code used for these classes from:
 *   http://www.stjarnhimlen.se/comp/tutorial.html
 *   http://www.stjarnhimlen.se/comp/riset.html
 *   http://www.srrb.noaa.gov/highlights/solarrise/azel.html
 *   http://www.srrb.noaa.gov/highlights/sunrise/sunrise.html
 *   http://bodmas.org/astronomy/riset.html
 *   moontool.c by John Walker
 *   Wikipedia
 */

class SolarSystemObject
{
    public:
        SolarSystemObject();
        virtual ~SolarSystemObject();

        double meanLongitude() const { return L; };
        double meanAnomaly() const { return M; };
        double siderealTime();
        double altitude() const { return m_altitude; };
        double azimuth() const { return m_azimuth; };
        double calcElevation();
        QDateTime dateTime() const { return m_local; };
        double lambda() const { return m_lambda; };
        double eclipticLongitude() const { return m_eclipticLongitude; };
        void setPosition(double latitude, double longitude);

        virtual void calcForDateTime(const QDateTime& local, int offset);
        QList< QPair<QDateTime, QDateTime> > timesForAngles(const QList<double>& angles,
                                                            const QDateTime& dt,
                                                            int offset);

    protected:
        void calc();
        virtual bool calcPerturbations(double*, double*, double*) { return false; };
        virtual void rotate(double*, double*) { };
        virtual void topocentricCorrection(double*, double*) { };

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
        virtual void calcForDateTime(const QDateTime& local, int offset);

    protected:
        virtual void rotate(double*, double*);
};

class Moon : public SolarSystemObject
{
    public:
        Moon(Sun *sun);
        virtual ~Moon() {}; // to not delete the Sun

        virtual void calcForDateTime(const QDateTime& local, int offset);
        double phase();

    protected:
        virtual bool calcPerturbations(double *RA, double *dec, double *r);
        virtual void rotate(double*, double*);
        virtual void topocentricCorrection(double*, double*);

    private:
        Sun *m_sun;
};

#endif
