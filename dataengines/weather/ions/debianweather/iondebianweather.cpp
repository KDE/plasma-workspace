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
#define QT_NO_CAST_FROM_ASCII

#include "iondebianweather.h"

#include <KIO/TransferJob>
#include <KIO/Job>

#include <KUnitConversion/Converter>

#include <QXmlStreamReader>

IonDebianWeather::IonDebianWeather(QObject*, const QVariantList&) : IonInterface(), m_ionname(QLatin1String("DebianWeather")) {

}

void IonDebianWeather::init() {
  setInitialized(true);
}

IonDebianWeather::~IonDebianWeather() {
  cleanup();
}

void IonDebianWeather::reset() {
  cleanup();
}

void IonDebianWeather::cleanup() {
  Q_FOREACH(KJob* job, m_jobs.keys()) {
    delete job;
  }
  m_jobs.clear();
}

IonDebianWeather::jobtype IonDebianWeather::toJobType(const QString& data) {
  if(data==QLatin1String("validate")) {
    return validate;
  }
  if(data==QLatin1String("weather")) {
    return weather;
  }
  return unknown;
}

QString IonDebianWeather::toString(IonDebianWeather::jobtype type) {
  switch(type) {
    case validate:
      return QLatin1String("validate");
      break;
      ;;
    case weather:
      return QLatin1String("weather");
      break;
      ;;
    case unknown:
      ;;
  }
  return QLatin1String("unknown");
}

bool IonDebianWeather::updateIonSource(const QString& source) {
        // We expect the applet to send the source in the following tokenization:
        // ionname|validate|place_name - Triggers validation of place
        // ionname|weather|place_name - Triggers receiving weather of place
  QStringList tokens = source.split(QLatin1Char('|'));

  if(tokens.count()==3 && toJobType(tokens.at(1)) == validate) {
    //validate, whatever that is.
    QSharedPointer<locationdata> p(new locationdata());
    if (tokens.at(2).simplified().startsWith(QLatin1String("Debian"), Qt::CaseInsensitive)){
      p->source = source;
      p->type = validate;
      p->valid = true;
      QStringList searchparts = tokens.at(2).simplified().split(QLatin1Char(' '));
      if(searchparts.size() > 1) {
        QStringList components(searchparts);
        components.pop_front();
        p->searchstring = components.join(QLatin1String(" "));
      }
      findAllPlaces(p);
      return true;
    }else {
      setData(source,
              toString(validate),
              QString::fromLatin1("%1|invalid|single|%2").arg(m_ionname).arg(tokens.at(2).simplified()));
      return true;
    }
    return true;
  } else if(tokens.count()==3 && toJobType(tokens.at(1))==weather) {
    QSharedPointer<locationdata> p(new locationdata());
    parseLocation(tokens.at(2).simplified(),p);
    if(p->valid) {
      p->source = source;
      p->type = weather;
      startFetchData(p);
      return true;
    } else {
      setData(source, toString(validate),QString::fromLatin1("%1|invalid|single|%2").arg(m_ionname).arg(tokens.at(2).simplified()));
      return true;
    }
  } else {
    setData(source,toString(validate),QString::fromLatin1("%1|malformed").arg(m_ionname));
    return true;
  }
  return false;
}

void IonDebianWeather::parseLocation(QString location,QSharedPointer<locationdata> data) {
  QStringList parts = location.split(QLatin1Char(' '));
  if(parts.size()==3 && parts.at(0)==QLatin1String("Debian")) {
    data->suite=parts.at(1);
    data->arch=parts.at(2);
    data->valid=true;
  }
}

void IonDebianWeather::findAllPlaces(QSharedPointer< IonDebianWeather::locationdata > data) {
  Q_ASSERT(data->type==validate);
  QUrl url (QLatin1String("http://edos.debian.net/edos-debcheck/results/available.xml"));
  KIO::TransferJob* job = KIO::get(url,KIO::NoReload,KIO::HideProgressInfo);
  if(job) {
    m_jobs[job]=data;
    connect(job,SIGNAL(result(KJob*)),this,SLOT(jobDone(KJob*)));
    connect(job,SIGNAL(data(KIO::Job*,QByteArray)),this,SLOT(dataReceived(KIO::Job*,QByteArray)));
  }
}

void IonDebianWeather::startFetchData(QSharedPointer<locationdata> data) {
  Q_ASSERT(data->valid);
  Q_ASSERT(data->type==weather);
  QUrl url(QString::fromLatin1("http://edos.debian.net/edos-debcheck/results/%1/latest/%2/weather.xml").arg(data->suite).arg(data->arch));
  KIO::TransferJob* job = KIO::get(url,KIO::NoReload,KIO::HideProgressInfo);
  if(job) {
    m_jobs[job]=data;
    connect(job,SIGNAL(result(KJob*)),this,SLOT(jobDone(KJob*)));
    connect(job,SIGNAL(data(KIO::Job*,QByteArray)),this,SLOT(dataReceived(KIO::Job*,QByteArray)));
  }
}

void IonDebianWeather::jobDone(KJob* job) {
  if(KIO::TransferJob* kiojob = qobject_cast<KIO::TransferJob*>(job)) {
    QSharedPointer<locationdata> locdata = m_jobs.value(job);
    if(locdata->valid && locdata->type==weather) {
      if(kiojob->error() != 0 || kiojob->isErrorPage()) {
        setData(locdata->source, toString(validate), QString::fromLatin1("%1|timeout").arg(m_ionname));
      } else {
        xmlresult result = parseWeatherXml(locdata);
        ConditionIcons cond = toCondition(result);
        Plasma::DataEngine::Data data;
        data.insert( QLatin1String("Condition Icon"), getWeatherIcon(cond) );
        data.insert( QLatin1String("Place"), result.nicename );
        data.insert( QLatin1String("Temperature"), QString::number(result.broken) );
        data.insert( QLatin1String("Temperature Unit"), QString::number(KUnitConversion::Celsius) );
        data.insert( QLatin1String("Total Weather Days"), 0 );
        data.insert( QLatin1String("Current Conditions"), result.architecture );
        data.insert( QLatin1String("Credit"), QString::fromLatin1("Data from http://edos.debian.net"));
        data.insert( QLatin1String("Credit Url"), QString::fromLatin1("http://edos.debian.net"));
        setData( locdata->source,data);
      }
    } else if( locdata->type == validate ) {
      if(kiojob->error()!=0 || kiojob->isErrorPage()) {
        setData(locdata->source, toString(validate),QString::fromLatin1("%1|invalid|single|Debian %2").arg(m_ionname).arg(locdata->searchstring).simplified());
      } else {
        QStringList archsuitelist = parseLocationXml(locdata);
        QString places;
        if(archsuitelist.size()==0) {
          setData(locdata->source, toString(validate),QString::fromLatin1("%1|invalid|single|Debian %2").arg(m_ionname).arg(locdata->searchstring).simplified());
        } else if(archsuitelist.size()==1) {
          setData(locdata->source, toString(validate), QString::fromLatin1("%1|valid|single|place|Debian %2").arg(m_ionname).arg(locdata->searchstring).simplified());
        } else {
          Q_FOREACH(QString place, archsuitelist) {
            places += QLatin1String("|place|");
            places += place;
          }
          setData(locdata->source, toString(validate), QString::fromLatin1("%1|valid|multiple%2").arg(m_ionname).arg(places));
        }
      }
    } else {
      kError() << "something went horribly wrong with this locdata";
    }
    m_jobs.remove(kiojob);
    kiojob->deleteLater();
  }
}

void IonDebianWeather::dataReceived(KIO::Job* job, QByteArray data) {
  QSharedPointer<locationdata> locdata = m_jobs.value(job);
  if(!locdata.isNull() && locdata->valid) {
    locdata->data.append(data);
  } else {
    kError() << "wtf";
  }
}

IonDebianWeather::xmlresult IonDebianWeather::parseWeatherXml(QSharedPointer< IonDebianWeather::locationdata > locdata) {
  QXmlStreamReader reader(locdata->data);
  xmlresult result;
  while(!reader.atEnd()) {
    reader.readNextStartElement();
    if( reader.name() == QLatin1String("total")) {
      result.total = reader.readElementText().trimmed().toInt();
    } else if( reader.name() == QLatin1String("broken") ) {
      result.broken = reader.readElementText().trimmed().toInt();
    } else if(reader.name() == QLatin1String("description") ) {
      result.nicename = reader.readElementText();
    } else if(reader.name() == QLatin1String("architecture") ) {
      result.architecture = reader.readElementText();
    }
  }
  return result;
}

QStringList IonDebianWeather::parseLocationXml(QSharedPointer< IonDebianWeather::locationdata > locdata) {
  QXmlStreamReader reader(locdata->data);
  QStringList rv;
  while (!reader.atEnd()) {
    reader.readNext();
    while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == QLatin1String("weathers"))) {
      reader.readNext();
      QString suite;
      while(!reader.atEnd() && !(reader.isEndElement() && reader.name()==QLatin1String("weather"))) {
        reader.readNext();
        if(reader.isStartElement() && reader.name() == QLatin1String("name")) {
          suite = reader.readElementText().trimmed();
          continue;
        }
        if(reader.isStartElement() && reader.name() == QLatin1String("title")) {
         // qDebug() << reader.readElementText();
          continue;
        }
        if(reader.isStartElement() && reader.name() == QLatin1String("archs")) {
          while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == QLatin1String("archs"))) {
            reader.readNext();
            if(reader.isStartElement() && reader.name()== QLatin1String("arch")) {
              QString arch = reader.readElementText();
              QString locstring = QString::fromLatin1("Debian %1 %2").arg(suite).arg(arch);
              if(locstring.contains(locdata->searchstring)) {
                rv << locstring;
              }
            }
          }
        }
      }
    }
  }
  return rv;
}

IonInterface::ConditionIcons IonDebianWeather::toCondition(IonDebianWeather::xmlresult result ) {
  qreal percentage=101; //magicvalue !
  if(result.total!=0) {
    percentage = result.broken*qreal(100.0)/result.total;
  }
  if(percentage <= 1) {
    return ClearDay;
  } else if(percentage <=2) {
    return FewCloudsDay;
  } else if(percentage <=3) {
    return Overcast;
  } else if(percentage <=4) {
    return Showers;
  } else if(percentage <=100) {
    return Thunderstorm;
  }
  return NotAvailable;
}

#include "iondebianweather.moc"
