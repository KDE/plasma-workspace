/*******************************************************************
* bugzillalib.cpp
* Copyright  2009, 2011  Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright  2012  George Kiagiadakis <kiagiadakis.george@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#include "bugzillalib.h"

#include <QtCore/QTextStream>
#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <QtXml/QDomNode>
#include <QtXml/QDomNodeList>
#include <QtXml/QDomElement>
#include <QtXml/QDomNamedNodeMap>

#include <KIO/Job>
#include <KLocalizedString>
#include <QDebug>


static const char columns[] = "bug_severity,priority,bug_status,product,short_desc,resolution";

//Bugzilla URLs
static const char searchUrl[] =
        "buglist.cgi?query_format=advanced&order=Importance&ctype=csv"
        "&product=%1"
        "&longdesc_type=allwordssubstr&longdesc=%2"
        "&chfieldfrom=%3&chfieldto=%4&chfield=[Bug+creation]"
        "&bug_severity=%5"
        "&columnlist=%6";
// short_desc, product, long_desc(possible backtraces lines), searchFrom, searchTo, severity, columnList
static const char showBugUrl[] = "show_bug.cgi?id=%1";
static const char fetchBugUrl[] = "show_bug.cgi?id=%1&ctype=xml";

static inline Component buildComponent(const QVariantMap& map);
static inline Version buildVersion(const QVariantMap& map);
static inline Product buildProduct(const QVariantMap& map);

//BEGIN BugzillaManager

BugzillaManager::BugzillaManager(const QString &bugTrackerUrl, QObject *parent)
        : QObject(parent)
        , m_bugTrackerUrl(bugTrackerUrl)
        , m_logged(false)
        , m_searchJob(0)
{
    m_xmlRpcClient = new KXmlRpc::Client(QUrl(m_bugTrackerUrl + "xmlrpc.cgi"), this);
    m_xmlRpcClient->setUserAgent(QLatin1String("DrKonqi"));
}

//BEGIN Login methods
void BugzillaManager::tryLogin(const QString& username, const QString& password)
{
    m_username = username;
    m_logged = false;

    QMap<QString, QVariant> args;
    args.insert(QLatin1String("login"), username);
    args.insert(QLatin1String("password"), password);
    args.insert(QLatin1String("remember"), false);

    m_xmlRpcClient->call(QLatin1String("User.login"), args,
            this, SLOT(callMessage(QList<QVariant>,QVariant)),
            this, SLOT(callFault(int,QString,QVariant)),
            QString::fromAscii("login"));
}

bool BugzillaManager::getLogged() const
{
    return m_logged;
}

QString BugzillaManager::getUsername() const
{
    return m_username;
}
//END Login methods

//BEGIN Bugzilla Action methods
void BugzillaManager::fetchBugReport(int bugnumber, QObject * jobOwner)
{
    QUrl url(m_bugTrackerUrl + QString(fetchBugUrl).arg(bugnumber));

    if (!jobOwner) {
        jobOwner = this;
    }

    KIO::Job * fetchBugJob = KIO::storedGet(url, KIO::Reload, KIO::HideProgressInfo);
    fetchBugJob->setParent(jobOwner);
    connect(fetchBugJob, SIGNAL(finished(KJob*)) , this, SLOT(fetchBugJobFinished(KJob*)));
}


void BugzillaManager::searchBugs(const QStringList & products,
                                const QString & severity, const QString & date_start,
                                const QString & date_end, QString comment)
{
    QString product;
    if (products.size() > 0) {
        if (products.size() == 1) {
            product = products.at(0);
        } else  {
            Q_FOREACH(const QString & p, products) {
                product += p + "&product=";
            }
            product = product.mid(0,product.size()-9);
        }
    }

    QString url = QString(m_bugTrackerUrl) +
                  QString(searchUrl).arg(product, comment.replace(' ' , '+'), date_start,
                                         date_end, severity, QString(columns));

    stopCurrentSearch();

    m_searchJob = KIO::storedGet(QUrl(url) , KIO::Reload, KIO::HideProgressInfo);
    connect(m_searchJob, SIGNAL(finished(KJob*)) , this, SLOT(searchBugsJobFinished(KJob*)));
}

void BugzillaManager::sendReport(const BugReport & report)
{
    QMap<QString, QVariant> args;
    args.insert(QLatin1String("product"), report.product());
    args.insert(QLatin1String("component"), report.component());
    args.insert(QLatin1String("version"), report.version());
    args.insert(QLatin1String("summary"), report.shortDescription());
    args.insert(QLatin1String("description"), report.description());
    args.insert(QLatin1String("op_sys"), report.operatingSystem());
    args.insert(QLatin1String("platform"), report.platform());
    args.insert(QLatin1String("keywords"), report.keywords());
    args.insert(QLatin1String("priority"), report.priority());
    args.insert(QLatin1String("severity"), report.bugSeverity());

    m_xmlRpcClient->call(QLatin1String("Bug.create"), args,
            this, SLOT(callMessage(QList<QVariant>,QVariant)),
            this, SLOT(callFault(int,QString,QVariant)),
            QString::fromAscii("Bug.create"));
}

void BugzillaManager::attachTextToReport(const QString & text, const QString & filename,
    const QString & summary, int bugId, const QString & comment)
{
    QMap<QString, QVariant> args;
    args.insert(QLatin1String("ids"), QVariantList() << bugId);
    args.insert(QLatin1String("file_name"), filename);
    args.insert(QLatin1String("summary"), summary);
    args.insert(QLatin1String("comment"), comment);
    args.insert(QLatin1String("content_type"), QString::fromAscii("text/plain"));

    //data needs to be a QByteArray so that it is encoded in base64 (query.cpp:246)
    args.insert(QLatin1String("data"), text.toUtf8());

    m_xmlRpcClient->call(QLatin1String("Bug.add_attachment"), args,
            this, SLOT(callMessage(QList<QVariant>,QVariant)),
            this, SLOT(callFault(int,QString,QVariant)),
            QString::fromAscii("Bug.add_attachment"));
}

void BugzillaManager::addMeToCC(int bugId)
{
    QMap<QString, QVariant> args;
    args.insert(QLatin1String("ids"), QVariantList() << bugId);

    QMap<QString, QVariant> ccChanges;
    ccChanges.insert(QLatin1String("add"), QVariantList() << m_username);
    args.insert(QLatin1String("cc"), ccChanges);

    m_xmlRpcClient->call(QLatin1String("Bug.update"), args,
            this, SLOT(callMessage(QList<QVariant>,QVariant)),
            this, SLOT(callFault(int,QString,QVariant)),
            QString::fromAscii("Bug.update.cc"));
}

void BugzillaManager::fetchProductInfo(const QString & product)
{
    QMap<QString, QVariant> args;

    args.insert("names", (QStringList() << product) ) ;

    QStringList includeFields;
    // currently we only need these informations
    includeFields << "name" << "is_active" << "components" << "versions";

    args.insert("include_fields", includeFields) ;

    m_xmlRpcClient->call(QLatin1String("Product.get"), args,
            this, SLOT(callMessage(QList<QVariant>,QVariant)),
            this, SLOT(callFault(int,QString,QVariant)),
            QString::fromAscii("Product.get.versions"));
}


//END Bugzilla Action methods

//BEGIN Misc methods
QString BugzillaManager::urlForBug(int bug_number) const
{
    return QString(m_bugTrackerUrl) + QString(showBugUrl).arg(bug_number);
}

void BugzillaManager::stopCurrentSearch()
{
    if (m_searchJob) { //Stop previous searchJob
        m_searchJob->disconnect();
        m_searchJob->kill();
        m_searchJob = 0;
    }
}
//END Misc methods

//BEGIN Slots to handle KJob::finished

void BugzillaManager::fetchBugJobFinished(KJob* job)
{
    if (!job->error()) {
        KIO::StoredTransferJob * fetchBugJob = static_cast<KIO::StoredTransferJob*>(job);

        BugReportXMLParser * parser = new BugReportXMLParser(fetchBugJob->data());
        BugReport report = parser->parse();

        if (parser->isValid()) {
            emit bugReportFetched(report, job->parent());
        } else {
            emit bugReportError(i18nc("@info","Invalid report information (malformed data). This "
                                      "could mean that the bug report does not exist, or the "
                                      "bug tracking site is experiencing a problem."), job->parent());
        }

        delete parser;
    } else {
        emit bugReportError(job->errorString(), job->parent());
    }
}

void BugzillaManager::searchBugsJobFinished(KJob * job)
{
    if (!job->error()) {
        KIO::StoredTransferJob * searchBugsJob = static_cast<KIO::StoredTransferJob*>(job);

        BugListCSVParser * parser = new BugListCSVParser(searchBugsJob->data());
        BugMapList list = parser->parse();

        if (parser->isValid()) {
            emit searchFinished(list);
        } else {
            emit searchError(i18nc("@info","Invalid bug list: corrupted data"));
        }

        delete parser;
    } else {
        emit searchError(job->errorString());
    }

    m_searchJob = 0;
}

static inline Component buildComponent(const QVariantMap& map)
{
    QString name = map.value("name").toString();
    bool active = map.value("is_active").toBool();

    return Component(name, active);
}

static inline Version buildVersion(const QVariantMap& map)
{
    QString name = map.value("name").toString();
    bool active = map.value("is_active").toBool();

    return Version(name, active);
}

static inline Product buildProduct(const QVariantMap& map)
{
    QString name = map.value("name").toString();
    bool active = map.value("is_active").toBool();

    Product product(name, active);

    QVariantList components = map.value("components").toList();
    foreach (const QVariant& c, components) {
        Component component = buildComponent(c.toMap());
        product.addComponent(component);

    }

    QVariantList versions = map.value("versions").toList();
    foreach (const QVariant& v, versions) {
        Version version = buildVersion(v.toMap());
        product.addVersion(version);
    }

    return product;
}

void BugzillaManager::fetchProductInfoFinished(const QVariantMap & map)
{
    QList<Product> products;

    QVariantList plist = map.value("products").toList();
    foreach (const QVariant& p, plist) {
        Product product = buildProduct(p.toMap());
        products.append(product);
    }

    if ( products.size() > 0 ) {
        emit productInfoFetched(products.at(0));
    } else {
        emit productInfoError();
    }
}

//END Slots to handle KJob::finished

void BugzillaManager::callMessage(const QList<QVariant> & result, const QVariant & id)
{
    qDebug() << id << result;

    if (id.toString() == QLatin1String("login")) {
        m_logged = true;
        Q_EMIT loginFinished(true);
    } else if (id.toString() == QLatin1String("Product.get.versions")) {
        QVariantMap map = result.at(0).toMap();
        fetchProductInfoFinished(map);
    } else if (id.toString() == QLatin1String("Bug.create")) {
        QVariantMap map = result.at(0).toMap();
        int bug_id = map.value(QLatin1String("id")).toInt();
        Q_ASSERT(bug_id != 0);
        Q_EMIT reportSent(bug_id);
    } else if (id.toString() == QLatin1String("Bug.add_attachment")) {
        QVariantMap map = result.at(0).toMap();
        if (map.contains(QLatin1String("attachments"))){  // for bugzilla 4.2
            map = map.value(QLatin1String("attachments")).toMap();
            map = map.constBegin()->toMap();
            const int attachment_id = map.value(QLatin1String("id")).toInt();
            Q_EMIT attachToReportSent(attachment_id);
        } else if (map.contains(QLatin1String("ids"))) {  // for bugzilla 4.4
            const int attachment_id = map.value(QLatin1String("ids")).toList().at(0).toInt();
            Q_EMIT attachToReportSent(attachment_id);
        }
    } else if (id.toString() == QLatin1String("Bug.update.cc")) {
        QVariantMap map = result.at(0).toMap().value(QLatin1String("bugs")).toList().at(0).toMap();
        int bug_id = map.value(QLatin1String("id")).toInt();
        Q_ASSERT(bug_id != 0);
        Q_EMIT addMeToCCFinished(bug_id);
    }
}

void BugzillaManager::callFault(int errorCode, const QString & errorString, const QVariant & id)
{
    qDebug() << id << errorCode << errorString;

    QString genericError = i18nc("@info", "Received unexpected error code %1 from bugzilla. "
                                 "Error message was: %2", errorCode, errorString);

    if (id.toString() == QLatin1String("login")) {
        switch(errorCode) {
        case 300: //invalid username or password
            Q_EMIT loginFinished(false); //TODO replace with loginError
            break;
        default:
            Q_EMIT loginError(genericError);
            break;
        }
    } else if (id.toString() == QLatin1String("Bug.create")) {
        switch (errorCode) {
        case 51:  //invalid object (one example is invalid platform value)
        case 105: //invalid component
        case 106: //invalid product
            Q_EMIT sendReportErrorInvalidValues();
            break;
        default:
            Q_EMIT sendReportError(genericError);
            break;
        }
    } else if (id.toString() == QLatin1String("Bug.add_attachment")) {
        switch (errorCode) {
        default:
            Q_EMIT attachToReportError(genericError);
            break;
        }
    } else if (id.toString() == QLatin1String("Bug.update.cc")) {
        switch (errorCode) {
        default:
            Q_EMIT addMeToCCError(genericError);
            break;
        }
    }
}

//END BugzillaManager

//BEGIN BugzillaCSVParser

BugListCSVParser::BugListCSVParser(const QByteArray& data)
{
    m_data = data;
    m_isValid = false;
}

BugMapList BugListCSVParser::parse()
{
    BugMapList list;

    if (!m_data.isEmpty()) {
        //Parse buglist CSV
        QTextStream ts(&m_data);
        QString headersLine = ts.readLine().remove(QLatin1Char('\"')) ;   //Discard headers
        QString expectedHeadersLine = QString(columns);

        if (headersLine == (QString("bug_id,") + expectedHeadersLine)) {
            QStringList headers = expectedHeadersLine.split(',', QString::KeepEmptyParts);
            int headersCount = headers.count();

            while (!ts.atEnd()) {
                BugMap bug; //bug report data map

                QString line = ts.readLine();

                //Get bug_id (always at first column)
                int bug_id_index = line.indexOf(',');
                QString bug_id = line.left(bug_id_index);
                bug.insert("bug_id", bug_id);

                line = line.mid(bug_id_index + 2);

                QStringList fields = line.split(",\"");

                for (int i = 0; i < headersCount && i < fields.count(); i++) {
                    QString field = fields.at(i);
                    field = field.left(field.size() - 1) ;   //Remove trailing "
                    bug.insert(headers.at(i), field);
                }

                list.append(bug);
            }

            m_isValid = true;
        }
    }

    return list;
}

//END BugzillaCSVParser

//BEGIN BugzillaXMLParser

BugReportXMLParser::BugReportXMLParser(const QByteArray & data)
{
    m_valid = m_xml.setContent(data, true);
}

BugReport BugReportXMLParser::parse()
{
    BugReport report; //creates an invalid and empty report object

    if (m_valid) {
        //Check bug notfound
        QDomNodeList bug_number = m_xml.elementsByTagName("bug");
        QDomNode d = bug_number.at(0);
        QDomNamedNodeMap a = d.attributes();
        QDomNode d2 = a.namedItem("error");
        m_valid = d2.isNull();

        if (m_valid) {
            report.setValid(true);

            //Get basic fields
            report.setBugNumber(getSimpleValue("bug_id"));
            report.setShortDescription(getSimpleValue("short_desc"));
            report.setProduct(getSimpleValue("product"));
            report.setComponent(getSimpleValue("component"));
            report.setVersion(getSimpleValue("version"));
            report.setOperatingSystem(getSimpleValue("op_sys"));
            report.setBugStatus(getSimpleValue("bug_status"));
            report.setResolution(getSimpleValue("resolution"));
            report.setPriority(getSimpleValue("priority"));
            report.setBugSeverity(getSimpleValue("bug_severity"));
            report.setMarkedAsDuplicateOf(getSimpleValue("dup_id"));
            report.setVersionFixedIn(getSimpleValue("cf_versionfixedin"));

            //Parse full content + comments
            QStringList m_commentList;
            QDomNodeList comments = m_xml.elementsByTagName("long_desc");
            for (int i = 0; i < comments.count(); i++) {
                QDomElement element = comments.at(i).firstChildElement("thetext");
                m_commentList << element.text();
            }

            report.setComments(m_commentList);

        } //isValid
    } //isValid

    return report;
}

QString BugReportXMLParser::getSimpleValue(const QString & name)   //Extract an unique tag from XML
{
    QString ret;

    QDomNodeList bug_number = m_xml.elementsByTagName(name);
    if (bug_number.count() == 1) {
        QDomNode node = bug_number.at(0);
        ret = node.toElement().text();
    }
    return ret;
}

//END BugzillaXMLParser

void BugReport::setBugStatus(const QString &stat)
{
    setData("bug_status", stat);

    m_status = parseStatus(stat);
}

void BugReport::setResolution(const QString &res)
{
    setData("resolution", res);

    m_resolution = parseResolution(res);
}

BugReport::Status BugReport::parseStatus(const QString &stat)
{
    if (stat == QLatin1String("UNCONFIRMED")) {
        return Unconfirmed;
    } else if (stat == QLatin1String("CONFIRMED")) {
        return New;
    } else if (stat == QLatin1String("ASSIGNED")) {
        return Assigned;
    } else if (stat == QLatin1String("REOPENED")) {
        return Reopened;
    } else if (stat == QLatin1String("RESOLVED")) {
        return Resolved;
    } else if (stat == QLatin1String("NEEDSINFO")) {
        return NeedsInfo;
    } else if (stat == QLatin1String("VERIFIED")) {
        return Verified;
    } else if (stat == QLatin1String("CLOSED")) {
        return Closed;
    } else {
        return UnknownStatus;
    }
}

BugReport::Resolution BugReport::parseResolution(const QString &res)
{
    if (res.isEmpty()) {
        return NotResolved;
    } else if (res == QLatin1String("FIXED")) {
        return Fixed;
    } else if (res == QLatin1String("INVALID")) {
        return Invalid;
    } else if (res == QLatin1String("WONTFIX")) {
        return WontFix;
    } else if (res == QLatin1String("LATER")) {
        return Later;
    } else if (res == QLatin1String("REMIND")) {
        return Remind;
    } else if (res == QLatin1String("DUPLICATE")) {
        return Duplicate;
    } else if (res == QLatin1String("WORKSFORME")) {
        return WorksForMe;
    } else if (res == QLatin1String("MOVED")) {
        return Moved;
    } else if (res == QLatin1String("UPSTREAM")) {
        return Upstream;
    } else if (res == QLatin1String("DOWNSTREAM")) {
        return Downstream;
    } else if (res == QLatin1String("WAITINGFORINFO")) {
        return WaitingForInfo;
    } else if (res == QLatin1String("BACKTRACE")) {
        return Backtrace;
    } else if (res == QLatin1String("UNMAINTAINED")) {
        return Unmaintained;
    } else {
        return UnknownResolution;
    }
}
