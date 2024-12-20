/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdlib>
#include <zlib.h>

#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QMap>
#include <QMimeData>
#include <QStandardPaths>
#include <QUrl>

#include "historymodel.h"
#include "systemclipboard.h"

using namespace Qt::StringLiterals;

void importHistory(const QString &history2FilePath)
{
    QFile historyFile(history2FilePath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        QCoreApplication::exit(EXIT_SUCCESS);
    }

    QDataStream fileStream(&historyFile);
    if (fileStream.atEnd()) {
        // The file is broken. No clips will be imported.
        QCoreApplication::exit(EXIT_SUCCESS);
    }

    QByteArray data;
    quint32 crc;
    fileStream >> crc >> data;
    if (crc32(0, reinterpret_cast<unsigned char *>(data.data()), data.size()) != crc) {
        QCoreApplication::exit(EXIT_SUCCESS);
    }

    QDataStream dataStream(&data, QIODevice::ReadOnly);
    char *version;
    dataStream >> version;
    delete[] version;

    auto model = HistoryModel::self();
    QList<std::shared_ptr<QMimeData>> dataList;

    while (!dataStream.atEnd()) {
        QString type;
        dataStream >> type;
        if (type == u"url") {
            QList<QUrl> urls;
            QMap<QString, QString> metaData;
            int cut;
            dataStream >> urls;
            dataStream >> metaData;
            dataStream >> cut;
            auto mimeData = std::make_shared<QMimeData>();
            mimeData->setUrls(urls);
            for (auto it = metaData.cbegin(); it != metaData.cend(); it = std::next(it)) {
                mimeData->setData(it.key(), it.value().toUtf8());
            }
            mimeData->setData(u"application/x-kde-cutselection"_s, QByteArray(cut ? "1" : "0"));
            dataList.push_back(mimeData);
        }
        if (type == u"string") {
            QString text;
            dataStream >> text;
            auto mimeData = std::make_shared<QMimeData>();
            mimeData->setText(text);
            dataList.push_back(mimeData);
        }
        if (type == u"image") {
            QImage image;
            dataStream >> image;
            auto mimeData = std::make_shared<QMimeData>();
            mimeData->setImageData(image);
            dataList.push_back(mimeData);
        }
    }

    qInfo() << "Size:" << dataList.size();
    qreal timestamp = QDateTime::currentSecsSinceEpoch();
    for (auto &it : std::as_const(dataList)) {
        model->insert(it.get(), timestamp--);
    }

    int count = 0;
    while (model->pendingJobs() > 0 && count++ < 100000) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    QCoreApplication::exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    QCommandLineOption inputOpt(QStringLiteral("input"), QStringLiteral("history2.lst"), QStringLiteral("input"));
    QCommandLineOption outputOpt(QStringLiteral("output"), QStringLiteral("history3.sqlite"), QStringLiteral("output"));
    parser.addOption(inputOpt);
    parser.addOption(outputOpt);
    parser.process(app);

    QString history2FilePath;
    if (parser.isSet(inputOpt)) {
        history2FilePath = parser.value(inputOpt);
    } else {
        history2FilePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"klipper/history2.lst"_s);
    }
    if (history2FilePath.isEmpty()) {
        return EXIT_SUCCESS;
    }

    QString history3FilePath;
    if (parser.isSet(outputOpt)) {
        history3FilePath = parser.value(outputOpt);
        qputenv("KLIPPER_DATABASE", history3FilePath.toLocal8Bit());
    } else {
        history3FilePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper/history3.sqlite";
    }
    if (QFile::exists(history3FilePath)) {
        // Already imported
        return EXIT_SUCCESS;
    }

    QMetaObject::invokeMethod(
        &app,
        [history2FilePath] {
            importHistory(history2FilePath);
        },
        Qt::QueuedConnection);

    return app.exec();
}
