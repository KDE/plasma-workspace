/*
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemclipboard.h"

#include <chrono>

#include <QApplication>
#include <QBuffer>
#include <QFile>
#include <QImageWriter>
#include <QMimeData>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QWidget>

#include <KCompositeJob>
#include <KIO/FileJob>
#include <kurlmimedata.h>

#include "../c_ptr.h"
#include "historyitem.h"
#include "klipper_debug.h"
#include "updateclipboardjob.h"
#include <wayland-client-core.h>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

namespace
{
// Protection against too many clipboard data changes. Lyx responds to clipboard data
// requests with setting new clipboard data, so if Lyx takes over clipboard,
// Klipper notices, requests this data, this triggers "new" clipboard contents
// from Lyx, so Klipper notices again, requests this data, ... you get the idea.
constexpr auto MAX_CLIPBOARD_CHANGES = 10; // max changes per second

bool ignoreClipboardChanges()
{
    // Changing a spinbox in klipper's config-dialog causes the lineedit-contents
    // of the spinbox to be selected and hence the clipboard changes. But we don't
    // want all those items in klipper's history. See #41917
    const auto app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app) {
        return false;
    }

    QWidget *focusWidget = app->focusWidget();
    if (focusWidget) {
        if (focusWidget->inherits("QSpinBox")
            || (focusWidget->parentWidget() && focusWidget->inherits("QLineEdit") && focusWidget->parentWidget()->inherits("QSpinWidget"))) {
            return true;
        }
    }

    return false;
}
}

class DatabaseRecordToMimeDataJob : public KCompositeJob
{
    Q_OBJECT

public:
    explicit DatabaseRecordToMimeDataJob(QObject *parent, const std::shared_ptr<const HistoryItem> &data);
    ~DatabaseRecordToMimeDataJob() override;

    void start() override;
    QMimeData *mimeData() const;

private:
    void slotResult(KJob *job) override;

    QString m_uuid;
    std::list<std::pair<QString /*type*/, QByteArray /*data*/>> m_mimeDataList;
};

DatabaseRecordToMimeDataJob::DatabaseRecordToMimeDataJob(QObject *parent, const std::shared_ptr<const HistoryItem> &data)
    : KCompositeJob(parent)
    , m_uuid(data->uuid())
{
}

DatabaseRecordToMimeDataJob::~DatabaseRecordToMimeDataJob() = default;

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
                m_mimeDataList.emplace_back(mimeType, data);
                job->close();
            });
            job->read(job->size());
        });
        addSubjob(job);
    }
    if (!hasSubjobs()) {
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

    if (hasSubjobs()) {
        return;
    }

    emitResult();
}

std::shared_ptr<SystemClipboard> SystemClipboard::self()
{
    static std::weak_ptr<SystemClipboard> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<SystemClipboard> ptr{new SystemClipboard};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

SystemClipboard::SystemClipboard()
    : QObject(nullptr)
    , m_clip(KSystemClipboard::instance())
{
    connect(m_clip, &KSystemClipboard::changed, this, [this](QClipboard::Mode mode) {
        checkClipData(mode);
    });

    m_pendingCheckTimer.setSingleShot(true);
    connect(&m_pendingCheckTimer, &QTimer::timeout, this, &SystemClipboard::slotCheckPending);
    connect(&m_overflowClearTimer, &QTimer::timeout, this, &SystemClipboard::slotClearOverflow);
}

SystemClipboard::~SystemClipboard() = default;

void SystemClipboard::checkClipData(QClipboard::Mode mode, const QMimeData *data)
{
    if ((mode == QClipboard::Clipboard && m_clipboardLocklevel) || (mode == QClipboard::Selection && m_selectionLocklevel)) {
        return;
    }

    Ignore lock(mode == QClipboard::Selection ? m_selectionLocklevel : m_clipboardLocklevel);

    // internal to klipper, ignoring QSpinBox selections
    if (ignoreClipboardChanges()) {
        // keep our old clipboard, thanks
        // This won't quite work, but it's close enough for now.
        // The trouble is that the top selection =! top clipboard
        // but we don't track that yet. We will....
        Q_EMIT ignored(mode);
        return;
    }

    qCDebug(KLIPPER_LOG) << "Checking clip data";

    if (!data) {
        data = m_clip->mimeData(mode);
    }

    if (!data) {
        Q_EMIT receivedEmptyClipboard(mode);
        return;
    } else if (data->formats().isEmpty()) {
        // The selection is valid but it has no data.
        qCDebug(KLIPPER_LOG) << "Empty selection. Nothing to synchronize";
        return;
    }

    if (!data->hasUrls() && !data->hasText() && !data->hasImage()) {
        return; // unknown, ignore
    }

    if (data->hasFormat(QStringLiteral("application/x-kde-syncselection"))) {
        return;
    }

    Q_ASSERT_X((mode == QClipboard::Clipboard && m_clipboardLocklevel == 1) || (mode == QClipboard::Selection && m_selectionLocklevel == 1),
               Q_FUNC_INFO,
               qPrintable(QStringLiteral("%1 %2").arg(QString::number(m_clipboardLocklevel), QString::number(m_selectionLocklevel))));
    Q_EMIT newClipData(mode, data);
}

void SystemClipboard::clear(SelectionMode mode)
{
    Q_ASSERT((mode & 1) == 0); // Warn if trying to pass a boolean as a mode.

    if (mode & Selection) {
        Ignore lock(m_selectionLocklevel);
        m_clip->clear(QClipboard::Selection);
    }
    if (mode & Clipboard) {
        Ignore lock(m_clipboardLocklevel);
        m_clip->clear(QClipboard::Clipboard);
    }
}

void SystemClipboard::setMimeData(const HistoryItemConstPtr &data, SelectionMode mode, ClipboardUpdateReason updateReason)
{
    Q_ASSERT((mode & 1) == 0); // Warn if trying to pass a boolean as a mode.
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        return;
    }

    auto job = new DatabaseRecordToMimeDataJob(this, data);
    connect(job, &KJob::finished, this, [this, job, mode, updateReason] {
        setMimeDataInternal(mode & Selection ? job->mimeData() : nullptr, mode & Clipboard ? job->mimeData() : nullptr, updateReason);
    });
    job->start();
}

void SystemClipboard::setMimeData(const QMimeData *data, SelectionMode mode, ClipboardUpdateReason updateReason)
{
    Q_ASSERT((mode & 1) == 0); // Warn if trying to pass a boolean as a mode.
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        return;
    }

    const QStringList formats = data->formats();
    auto setData = [data, &formats]() {
        auto mimeData = new QMimeData;
        if (data->hasImage()) {
            mimeData->setImageData(data->imageData());
        }
        if (data->hasText()) {
            mimeData->setText(data->text());
        }
        for (const QString &format : formats) {
            if (format.startsWith(s_plainTextPrefix) || format.startsWith(u"image/") || format == u"application/x-qt-image") {
                continue; // Already saved
            }
            mimeData->setData(format, data->data(format));
        }
        return mimeData;
    };
    QMimeData *selectionMimeData = nullptr;
    if (mode & Selection) {
        selectionMimeData = setData();
    }
    QMimeData *clipboardMimeData = nullptr;
    if (mode & Clipboard) {
        clipboardMimeData = setData();
    }

    setMimeDataInternal(selectionMimeData, clipboardMimeData, updateReason);
}

bool SystemClipboard::isLocked(QClipboard::Mode mode)
{
    return mode == QClipboard::Selection ? m_selectionLocklevel : m_clipboardLocklevel;
}

void SystemClipboard::slotClearOverflow()
{
    m_overflowClearTimer.stop();

    m_overflowCounter = 0;
}

void SystemClipboard::slotCheckPending()
{
    if (!m_pendingContentsCheck) {
        return;
    }
    m_pendingContentsCheck = false; // blockFetchingNewData() will be called again
    checkClipData(QClipboard::Selection); // Always selection
}

void SystemClipboard::setMimeDataInternal(QMimeData *selectionMimeData, QMimeData *clipboardMimeData, ClipboardUpdateReason updateReason)
{
    Q_ASSERT(qGuiApp);
    if (selectionMimeData) {
        Ignore lock(m_selectionLocklevel);
        if (updateReason == ClipboardUpdateReason::PreventEmptyClipboard) {
            selectionMimeData->setData(QStringLiteral("application/x-kde-onlyReplaceEmpty"), "1");
        }
        qCDebug(KLIPPER_LOG) << "Setting selection to <" << (selectionMimeData->hasImage() ? u"image"_s : selectionMimeData->text()) << ">";
        m_clip->setMimeData(selectionMimeData, QClipboard::Selection);
    }
    if (clipboardMimeData) {
        if (updateReason == ClipboardUpdateReason::PreventEmptyClipboard) {
            clipboardMimeData->setData(QStringLiteral("application/x-kde-onlyReplaceEmpty"), "1");
        } else if (updateReason == ClipboardUpdateReason::SyncSelection) {
            // When plasmashell is not focused, klipper will not receive new clip data immediately. This type is used to filter out selections.
            clipboardMimeData->setData(QStringLiteral("application/x-kde-syncselection"), "1");
        }
        QMetaObject::invokeMethod(
            this,
            [this, clipboardMimeData]() {
                Ignore lock(m_clipboardLocklevel);
                qCDebug(KLIPPER_LOG) << "Setting clipboard to <" << (clipboardMimeData->hasImage() ? u"image"_s : clipboardMimeData->text()) << ">";
                m_clip->setMimeData(clipboardMimeData, QClipboard::Clipboard);
            },
            updateReason == ClipboardUpdateReason::SyncSelection ? Qt::QueuedConnection : Qt::DirectConnection);
    }
}

#include "moc_systemclipboard.cpp"
#include "systemclipboard.moc"
