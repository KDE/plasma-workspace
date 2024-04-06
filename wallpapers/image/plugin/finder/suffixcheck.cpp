/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "suffixcheck.h"

#include <mutex>

#include <QImageReader>
#include <QMimeDatabase>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QSet>

namespace
{
constinit QStringList s_suffixes;
constinit std::once_flag s_onceFlag;

void fillSuffixes()
{
    Q_ASSERT(s_suffixes.empty());

    QSet<QString> suffixeSet;
    QMimeDatabase db;
    const auto supportedMimeTypes = QImageReader::supportedMimeTypes();

    for (const QByteArray &mimeType : supportedMimeTypes) {
        QMimeType mime(db.mimeTypeForName(QString::fromLatin1(mimeType)));
        const QStringList globPatterns = mime.globPatterns();

        for (const QString &pattern : globPatterns) {
            suffixeSet.insert(pattern);
        }
    }

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadFromModule("QtMultimedia", "MediaRecorder", QQmlComponent::PreferSynchronous);
    if (component.isReady()) {
        QObject *recorder = component.create(engine.rootContext());
        if (recorder) {
            QQmlExpression expr(engine.rootContext(), recorder, QStringLiteral("this.mediaFormat.supportedVideoCodecs(1).map(c => Number(c))"));
            const QVariant result = expr.evaluate();
            const QVariantList formats = result.value<QJSValue>().toVariant().toList();
            for (const QVariant &format : formats) {
                switch (format.toInt()) {
                case 0: // QMediaFormat::VideoCodec::MPEG1
                case 1: // QMediaFormat::VideoCodec::MPEG2
                    suffixeSet << QStringLiteral("*.ts");
                case 2: // QMediaFormat::VideoCodec::MPEG4
                case 3: // QMediaFormat::VideoCodec::H264
                case 4: // QMediaFormat::VideoCodec::H265
                    suffixeSet << QStringLiteral("*.m4a") << QStringLiteral("*.mkv") << QStringLiteral("*.mp4") << QStringLiteral("*.mov");
                case 5: // QMediaFormat::VideoCodec::VP8
                case 6: // QMediaFormat::VideoCodec::VP9
                    suffixeSet << QStringLiteral("*.webm");
                case 7: // QMediaFormat::VideoCodec::AV1
                    suffixeSet << QStringLiteral("*.av1");
                case 8: // QMediaFormat::VideoCodec::Theora
                    suffixeSet << QStringLiteral("*.ogv");
                case 9: // QMediaFormat::VideoCodec::WMV
                    suffixeSet << QStringLiteral("*.wmv");
                case 10: // QMediaFormat::VideoCodec::MotionJPEG
                    suffixeSet << QStringLiteral("*.mjpeg");
                }
            }
            delete recorder;
        }
    }
    s_suffixes = suffixeSet.values();
}
}

const QStringList &suffixes()
{
    std::call_once(s_onceFlag, &fillSuffixes);
    return s_suffixes;
}

bool isAcceptableSuffix(QString &&suffix)
{
    // Despite its name, suffixes() returns a list of glob patterns.
    // Therefore the file suffix check needs to include the "*." prefix.
    std::call_once(s_onceFlag, &fillSuffixes);
    return s_suffixes.contains(QLatin1String("*.%1").arg(std::move(suffix).toLower()));
}
