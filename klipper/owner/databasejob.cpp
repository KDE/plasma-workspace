
#include "databasejob.h"

#include <KIO/FileJob>

#include <QImage>
#include <QMimeData>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

using namespace Qt::StringLiterals;

inline constexpr QLatin1String s_imageFormat("image/png");

DatabaseRecordToMimeDataJob::DatabaseRecordToMimeDataJob(QObject *parent, const QString &uuid)
    : KCompositeJob(parent)
    , m_uuid(uuid)
{
}

DatabaseRecordToMimeDataJob::~DatabaseRecordToMimeDataJob()
{
}

void DatabaseRecordToMimeDataJob::start()
{
    QSqlDatabase db = QSqlDatabase::database(u"klipper"_s);
    if (!db.isOpen()) [[unlikely]] {
        setError(UserDefinedError);
        setErrorText(db.lastError().text());
        emitResult();
        return;
    }

    QSqlQuery query(db);
    query.exec(u"SELECT mimetype,data_uuid FROM aux WHERE uuid='%1'"_s.arg(m_uuid));
    while (query.next()) {
        const QString mimeType = query.value(0).toString();
        const QString dataUuid = query.value(1).toString();
        if (mimeType.isEmpty() || dataUuid.isEmpty()) {
            continue;
        }
        const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper/data/" + m_uuid + u'/' + dataUuid;
        auto job = KIO::open(QUrl::fromLocalFile(dataPath), QIODevice::ReadOnly);
        connect(job, &KIO::FileJob::open, this, [this, job, mimeType] {
            connect(job, &KIO::FileJob::data, this, [this, job, mimeType](KJob *, const QByteArray &data) {
                if (!data.isEmpty()) {
                    m_mimeDataList.emplace_back(mimeType, data);
                } else {
                    setError(UserDefinedError);
                    setErrorText(u"file does not exist"_s);
                    emitResult();
                }
                job->close();
            });
            job->read(job->size());
        });
        addSubjob(job);
    }
    if (!hasSubjobs()) {
        setError(UserDefinedError);
        setErrorText(u"uuid does not seem to exist"_s);
        emitResult();
        return;
    }
}

QMimeData *DatabaseRecordToMimeDataJob::mimeData() const
{
    auto mimeData = new QMimeData;
    for (auto &[format, data] : m_mimeDataList) {
        if (format == s_imageFormat) {
            mimeData->setImageData(QImage::fromData(data, "PNG"));
        } else {
            mimeData->setData(format, data);
        }
    }
    return mimeData;
}

void DatabaseRecordToMimeDataJob::slotResult(KJob *job)
{
    removeSubjob(job);

    if (hasSubjobs() && isFinished()) {
        return;
    }

    emitResult();
}
