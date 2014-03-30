/*
    Copyright (c) 2011 Sune Vuorela <sune@vuorela.dk>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef IONDEBIANWEATHER_H
#define IONDEBIANWEATHER_H

#include "../ion.h"
#include <Plasma/DataEngine>

class KJob;
namespace KIO {
  class Job;
}
class IonDebianWeather : public IonInterface {
  Q_OBJECT
  public:
    IonDebianWeather(QObject*, const QVariantList&);
    virtual ~IonDebianWeather();
    virtual bool updateIonSource(const QString& source);
    virtual void init();
  public Q_SLOTS:
    virtual void reset();
  private:
    enum jobtype {
      validate,
      weather,
      unknown
    };
    struct xmlresult {
      int total;
      int broken;
        QString nicename;
        QString architecture;
      xmlresult() : total(0), broken(0) {}
    };
    struct locationdata {
      QString suite;
      QString arch;
      bool valid;
      QString source;
      QByteArray data;
      jobtype type;
        QString searchstring;
      locationdata() : valid(false),type(unknown) {}
    };
    void tryFindPlace(QSharedPointer<locationdata> data);
    void startFetchData(QSharedPointer<locationdata> data);
    void findAllPlaces(QSharedPointer< IonDebianWeather::locationdata > data);
    void parseLocation(QString sourcedata, QSharedPointer<locationdata> location);
    QString m_ionname;
    QHash<KJob*,QSharedPointer<locationdata> > m_jobs;
    xmlresult parseWeatherXml(QSharedPointer<locationdata> locdata);
    QStringList parseLocationXml(QSharedPointer< IonDebianWeather::locationdata > locdata);

    jobtype toJobType(const QString& data);
    QString toString(jobtype type);
    ConditionIcons toCondition(xmlresult);
    void cleanup();
  private Q_SLOTS:
    void jobDone(KJob*);
    void dataReceived(KIO::Job* job,QByteArray data);
};

K_EXPORT_PLASMA_DATAENGINE(debianweather, IonDebianWeather)

#endif // IONDEBIANWEATHER_H
