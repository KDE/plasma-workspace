
#pragma once

#include <KCompositeJob>

class QMimeData;

class DatabaseRecordToMimeDataJob : public KCompositeJob
{
    Q_OBJECT

public:
    explicit DatabaseRecordToMimeDataJob(QObject *parent, const QString &uuid);
    ~DatabaseRecordToMimeDataJob() override;

    void start() override;
    QMimeData *mimeData() const;

private:
    void slotResult(KJob *job) override;

    QString m_uuid;
    std::list<std::pair<QString /*type*/, QByteArray /*data*/>> m_mimeDataList;
};
