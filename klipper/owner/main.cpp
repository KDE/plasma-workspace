
#include "../systemclipboard.h"
#include "databasejob.h"
#include "klipper_debug.h"

#include <KSystemClipboard>

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QMimeData>
#include <QSqlDatabase>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

using ClipboardUpdateReason = SystemClipboard::ClipboardUpdateReason;
using SelectionMode = SystemClipboard::SelectionMode;

QMimeData *enhanceMimeData(QMimeData *mimeData, ClipboardUpdateReason reason)
{
    if (reason == ClipboardUpdateReason::PreventEmptyClipboard) {
        mimeData->setData(QStringLiteral("application/x-kde-onlyReplaceEmpty"), "1");
    }
    if (reason == ClipboardUpdateReason::SyncSelection) {
        mimeData->setData(QStringLiteral("application/x-kde-syncselection"), "1");
    }
    return mimeData;
}

int main(int argc, char **argv)
{
    QCoreApplication::setQuitLockEnabled(false); // running a kjob...
    QGuiApplication app(argc, argv);
    QCommandLineParser parser;
    QStringList valueOptions = {u"mode"_s, u"reason"_s, u"uuid"_s};
    for (const auto &option : valueOptions) {
        parser.addOption(QCommandLineOption{option, {}, option});
    }
    parser.parse(app.arguments());
    bool ok = false;
    auto mode = static_cast<SelectionMode>(parser.value(u"mode"_s).toInt(&ok));
    if (!ok || !((mode & SelectionMode::Selection) || (mode & SelectionMode::Clipboard))) {
        qCWarning(KLIPPER_LOG) << "invalid mode" << ok << mode;
        return 1;
    }
    auto reason = static_cast<ClipboardUpdateReason>(parser.value(u"reason"_s).toInt(&ok));
    if (!ok) {
        qCWarning(KLIPPER_LOG) << "invalid reason" << ok << (int)reason;
        return 1;
    }

    auto uuid = parser.value(u"uuid"_s);
    if (uuid.isEmpty() && reason == ClipboardUpdateReason::SyncSelection) {
        auto syncTo = mode == SelectionMode::Clipboard ? QClipboard::Clipboard : QClipboard::Selection;
        auto syncFrom = mode == SelectionMode::Clipboard ? QClipboard::Selection : QClipboard::Clipboard;
        // Wait until we receive the data
        QEventLoop loop;
        auto waitConnection =
            QObject::connect(KSystemClipboard::instance(), &KSystemClipboard::changed, qGuiApp, [syncFrom, &loop](QClipboard::Mode changedMode) {
                if (changedMode == syncFrom) {
                    loop.exit();
                }
            });
        loop.exec();
        QObject::disconnect(waitConnection);

        auto source = KSystemClipboard::instance()->mimeData(syncFrom);
        auto copy = new QMimeData;
        if (source->hasImage()) {
            copy->setImageData(source->imageData());
        }
        if (source->hasText()) {
            copy->setText(source->text());
        }
        for (const QString &format : source->formats()) {
            if (format.startsWith("text/plain"_L1) || format.startsWith(u"image/") || format == u"application/x-qt-image") {
                continue; // Already saved
            }
            copy->setData(format, source->data(format));
        }
        KSystemClipboard::instance()->setMimeData(enhanceMimeData(copy, reason), syncTo);
        QObject::connect(KSystemClipboard::instance(), &KSystemClipboard::changed, qGuiApp, [syncTo, copy]() {
            if (KSystemClipboard::instance()->mimeData(syncTo) != copy) {
                qGuiApp->quit();
            }
        });
        return app.exec();
    }

    if (uuid.length() != 40) {
        qCWarning(KLIPPER_LOG) << "invalid uuid" << uuid;
        return 1;
    }

    auto db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"klipper"_s);
    db.setHostName(u"localhost"_s);
    if (qEnvironmentVariableIsSet("KLIPPER_DATABASE")) {
        db.setDatabaseName(qEnvironmentVariable("KLIPPER_DATABASE"));
    } else {
        db.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper/history3.sqlite");
    }

    auto job = new DatabaseRecordToMimeDataJob(qGuiApp, uuid);
    QObject::connect(job, &DatabaseRecordToMimeDataJob::result, qGuiApp, [job, mode, reason]() {
        if (job->error()) {
            qCCritical(KLIPPER_LOG) << job->errorText();
            qGuiApp->setQuitLockEnabled(true);
            qGuiApp->exit(1);
            return;
        }
        QMimeData *selectionMimeData = nullptr;
        QMimeData *clipboardMimeData = nullptr;
        // Note that 'job->mimeData' makes a new QMimeData object each time
        if (mode & SelectionMode::Selection) {
            selectionMimeData = enhanceMimeData(job->mimeData(), reason);
            KSystemClipboard::instance()->setMimeData(selectionMimeData, QClipboard::Selection);
        }
        if (mode & SelectionMode::Clipboard) {
            clipboardMimeData = enhanceMimeData(job->mimeData(), reason);
            KSystemClipboard::instance()->setMimeData(clipboardMimeData, QClipboard::Clipboard);
        }
        QObject::connect(KSystemClipboard::instance(), &KSystemClipboard::changed, qGuiApp, [selectionMimeData, clipboardMimeData] {
            if (KSystemClipboard::instance()->mimeData(QClipboard::Selection) != selectionMimeData
                && KSystemClipboard::instance()->mimeData(QClipboard::Clipboard) != clipboardMimeData) {
                qGuiApp->quit();
            }
        });
    });
    job->start();

    return app.exec();
}
