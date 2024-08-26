/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "historyitem.h"
#include "klipper_debug.h"
#include "mimetypes.h"

#include <KLocalizedString>
#include <kurlmimedata.h>

#include <QCryptographicHash>
#include <QIODevice>
#include <QMimeData>
#include <QMimeDatabase>

#include <ranges>

using namespace Qt::StringLiterals;

using RefView = std::ranges::ref_view<const MimeDataList>;
// std::ranges::key_view only works with lists of std::tuples or std::pairs
using KeysView = std::ranges::keys_view<RefView>;

// Get the key part of the key-value pair with more meaningful terminology.
inline const QString &key(const MimeData &mimeData)
{
    return mimeData.first;
}

// Get the value part of the key-value pair with more meaningful terminology.
// Automatically converts the type when requesting a different type.
template<typename T = const QVariant &>
inline T value(const MimeData &mimeData)
{
    if constexpr (std::is_same_v<std::decay_t<T>, QVariant>) {
        return mimeData.second;
    } else {
        return mimeData.second.value<T>();
    }
}

// QMap-like key-value pair finder.
inline MimeDataList::const_iterator find(const MimeDataList &list, const QString &mimetype)
{
    return std::find_if(list.begin(), list.end(), [&](const MimeData &mimeData) {
        return mimetype == key(mimeData);
    });
}

// QMap-like value getter.
// Automatically converts the type when requesting a different type.
template<typename T = const QVariant &>
inline T value(const MimeDataList &list, const QString &mimetype, const T &defaultValue = {})
{
    auto it = find(list, mimetype);
    return it != list.end() ? value<T>(*it) : defaultValue;
}

// QMap-like key-value pair insertion.
inline MimeDataList::const_reference insert(MimeDataList &list, MimeData &&mimeData)
{
    auto it = std::find_if(list.begin(), list.end(), [&](const MimeData &element) {
        return key(mimeData) == key(element);
    });
    if (it != list.cend()) {
        return *it = std::move(mimeData);
    }
    return list.emplace_back(std::move(mimeData));
}

template<typename T>
inline bool isEmpty(const T &arg)
{
    return arg.isEmpty();
}

template<>
inline bool isEmpty(const QImage &arg)
{
    return arg.isNull();
}

template<>
inline bool isEmpty(const QColor &arg)
{
    return !arg.isValid();
}

template<>
inline bool isEmpty(const QUrlList &arg)
{
    return arg.empty() || std::all_of(arg.begin(), arg.end(), [](const QUrl &url) {
               return url.isEmpty();
           });
}

template<>
inline bool isEmpty(const QVariant &arg)
{
    const auto typeId = arg.typeId();
    if (typeId == QMetaType::QString) {
        return isEmpty(arg.value<QString>());
    } else if (typeId == QMetaType::QByteArray) {
        return isEmpty(arg.value<QByteArray>());
    } else if (typeId == QMetaType::QImage) {
        return isEmpty(arg.value<QImage>());
    } else if (typeId == QMetaType::QColor) {
        return isEmpty(arg.value<QColor>());
    } else if (typeId == qMetaTypeId<QUrlList>()) {
        return isEmpty(arg.value<QUrlList>());
    }
    return true;
}

inline QByteArray computeUuid(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1);
}

inline QByteArray computeUuid(const MimeDataList &data)
{
    using namespace Mimetypes;
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    QMimeDatabase db;
    for (auto it = data.begin(); it != data.end(); ++it) {
        // Use known types.
        // We don't want custom application specific mimetypes to cause entries
        // that look like duplicates to users.
        // Apps that would cause visual duplicates include LibreOffice Calc.
        const auto &mimetype = key(*it);
        if (db.mimeTypeForName(mimetype).isValid() //
            || mimetype == Application::xQtImage //
            || mimetype == Application::xColor) {
            stream << *it;
        }
    }
    return computeUuid(buffer);
}

inline HistoryItemType getType(const MimeDataList &list)
{
    using namespace Mimetypes;
    // Most important to show since it can cause file operations.
    if (auto it = find(list, Text::uriList); it != list.cend()) {
        return HistoryItemType::Url;
    }
    // Next most important since it can be heavy and if you're copying an image
    // it's probably the main thing after file operations.
    if (auto it = find(list, Application::xQtImage); it != list.cend()) {
        return HistoryItemType::Image;
    }
    // Text is the safest and typically least heavy kind of thing to display.
    if (Utils::anyOfType(KeysView{list}, Type::text)) {
        return HistoryItemType::Text;
    }
    // We don't check for color because there doesn't seem to be a way to use it
    // in the clipboard in a practical way.
    return HistoryItemType::Invalid;
}

template<typename T>
inline QString getText(const T &arg)
{
    return QAnyStringView(arg).toString();
}

template<>
inline QString getText(const QString &arg)
{
    return arg;
}

template<>
inline QString getText(const QUrlList &urls)
{
    QString string = urls.value(0).toString(QUrl::FullyEncoded);
    for (int i = 1; i < urls.size(); ++i) {
        string += u' ' % urls[i].toString(QUrl::FullyEncoded);
    }
    return string;
}

template<>
inline QString getText(const QImage &image)
{
    if (image.isNull()) {
        return {};
    }
    return u"▨ " + i18n("%1x%2 %3bpp", image.width(), image.height(), image.depth());
}

template<>
inline QString getText(const QVariant &arg)
{
    const auto typeId = arg.typeId();
    if (typeId == QMetaType::QString) {
        return arg.value<QString>();
    } else if (typeId == QMetaType::QByteArray) {
        return QString::fromUtf8(arg.value<QByteArray>());
    } else if (typeId == QMetaType::QImage) {
        return getText(arg.value<QImage>());
    } else if (typeId == qMetaTypeId<QUrlList>()) {
        return getText(arg.value<QUrlList>());
    }
    return {};
}

template<>
inline QString getText(const MimeData &arg)
{
    return getText(value(arg));
}

template<>
inline QString getText(const MimeDataList &list)
{
    using namespace Mimetypes;
    // The best text format.
    if (auto it = find(list, Text::plain); it != list.cend()) {
        return getText(*it);
    }
    // A text format, but not as good for viewing in a plain format.
    if (auto it = find(list, Text::html); it != list.cend()) {
        return getText(*it);
    }
    // Could be anything, but still text
    for (auto it = list.cbegin(); it != list.cend(); ++it) {
        if (!value(*it).canConvert<QByteArray>() || !Utils::hasType(key(*it), Type::text)) {
            continue;
        }
        if (auto bytes = value<QByteArray>(*it); !bytes.isEmpty()) {
            return getText(bytes);
        }
    }
    // Technically text, but not really handled like text most of the time.
    if (auto it = find(list, Text::uriList); it != list.cend()) {
        return getText(*it);
    }
    // Least text-like known type.
    if (auto it = find(list, Application::xQtImage); it != list.cend()) {
        return getText(*it);
    }
    return {};
}

HistoryItem::HistoryItem(const MimeDataList &mimeData, HistoryItemType type, const QImage &image)
    : m_mimeDataList(mimeData)
    , m_uuid(computeUuid(m_mimeDataList))
    , m_type(type)
    , m_text(getText(mimeData))
    , m_image(image)
{
}

HistoryItem::~HistoryItem()
{
}

HistoryItemType HistoryItem::type() const
{
    return m_type;
}

QString HistoryItem::text() const
{
    return m_text;
}

QImage HistoryItem::image() const
{
    return m_image;
}

QMimeData *HistoryItem::newQMimeData() const
{
    using namespace Mimetypes;
    QMimeData *data = new QMimeData();
    bool hasUrls = false;
    for (auto it = m_mimeDataList.cbegin(); it != m_mimeDataList.cend(); ++it) {
        if (key(*it) == Text::uriList) {
            data->setUrls(value<QUrlList>(*it));
            hasUrls = true;
        } else if (key(*it) == Application::xQtImage) {
            data->setImageData(value(*it));
        } else if (key(*it) == Application::xColor) {
            data->setColorData(value(*it));
        } else if (key(*it) == Text::plain) {
            data->setText(value<QString>(*it));
        } else if (key(*it) == Text::html) {
            data->setText(value<QString>(*it));
        } else {
            data->setData(key(*it), value<QByteArray>(*it));
        }
    }
    if (hasUrls) {
        KUrlMimeData::exportUrlsToPortal(data);
    }
    return data;
}

void HistoryItem::write(QDataStream &stream) const
{
    stream << m_mimeDataList;
}

bool HistoryItem::operator==(const HistoryItem &rhs) const
{
    return m_mimeDataList == rhs.m_mimeDataList;
}

// Allows miscellaneous types and hints to be stored.
// We exclude image types because they would be super expensive.
// Most reported image types are actually generated on request, not stored.
inline bool isMisc(const QString &mimetype)
{
    using namespace Mimetypes;
    return !Utils::isImage(mimetype) //
        && mimetype != Application::xKdeOnlyReplaceEmpty //
        && mimetype != Application::xKdeSyncselection;
    // Exclude these hints because klipper adds them automatically.
}

HistoryItemPtr HistoryItem::create(const QMimeData *data, std::optional<QStringList> formats)
{
    using namespace Mimetypes;
    MimeDataList mimeDataList;
    if (!formats) {
        formats.emplace(data->formats());
    }
    // QMimeData can hold multiple types of data, so we check for all of them.
    // NOTE: The platform abstraction can change how QMimeData functions work.
    // In general, we want to set mime data in the order that it comes with.
    QImage image;
    for (const auto &format : std::as_const(*formats)) {
        if (format == Text::uriList) {
            QUrlList urls = KUrlMimeData::urlsFromMimeData(data, KUrlMimeData::PreferKdeUrls);
            if (!isEmpty(urls)) {
                insert(mimeDataList, {Text::uriList, QVariant::fromValue(std::move(urls))});
            }
        } else if (isEmpty(image) && Utils::isImage(format)) {
            // Convert whatever image format is there into a QImage and set it
            // as a QImage in our mime data map. We don't try to fetch every
            // image type because that would be super expensive. Most reported
            // image types are actually generated on request, not stored.
            // QInternalMimeData makes every format that can be generated by
            // QImageWriter available when you provide an image to the clipboard
            // as a QImage. The downside of only taking a QImage is that highly
            // compressed formats like JPEG will be decompressed. If you ask the
            // clipboard for an image format after the decompression, the image
            // will be recompressed, which could be lossy. Maybe we could only
            // store image data for formats in the list that come before
            // "application/x-qt-image", but it could be heavy on memory usage.
            // Only works if Klipper did not create the QMimeData.
            image = data->imageData().value<QImage>();
            // Needed for HistoryModelTest when created from an owned QMimeData
            // that sets raw image data for an image mimetype.
            if (isEmpty(image)) {
                image = QImage::fromData(data->data(format));
            }
            if (!isEmpty(image)) {
                insert(mimeDataList, {Application::xQtImage, image}); // Shallow copy
            }
        } else if (format == Application::xColor) {
            auto color = data->colorData().value<QColor>();
            if (!isEmpty(color)) {
                // fromValue() can do move operations, but QVariant() can't.
                insert(mimeDataList, {Application::xColor, QVariant::fromValue(std::move(color))});
            }
        } else if (Utils::isPlainText(format)) {
            QString text = data->text();
            if (!isEmpty(text)) {
                insert(mimeDataList, {Text::plain, QVariant::fromValue(std::move(text))});
            }
        } else if (format == Text::html) {
            // When copying rich text from a Qt app, you often get plain, html,
            // markdown and ODT formatted text.
            QString html = data->html();
            if (!isEmpty(html)) {
                insert(mimeDataList, {Text::html, QVariant::fromValue(std::move(html))});
            }
        } else if (Utils::hasType(format, Type::text)) {
            QByteArray bytes = data->data(format);
            if (!isEmpty(bytes)) {
                insert(mimeDataList, {format, QVariant::fromValue(std::move(bytes))});
            }
        } else if (isMisc(format)) {
            auto bytes = data->data(format);
            if (bytes.size() > 20'000'000) {
                // Skip anything greater than 20MB because we don't want too
                // many heavy things to be persistently held in the clipboard.
                continue;
            }
            // Some hints aren't set with data, so we keep them all.
            // Use an invalid QVariant when bytes are empty so that we don't
            // need to convert to the correct format first to check if valid.
            insert(mimeDataList, {format, isEmpty(bytes) ? QVariant{} : QVariant::fromValue(std::move(bytes))});
            // NOTE: Hopefully there are no platform abstractions that allow
            // stupidly expensive formats to be created when requested.
            // Images should be the worst that can happen (already handled above),
            // but in theory a platform abstraction could do almost anything.
        }
    }
    if (auto type = getType(mimeDataList); type != HistoryItemType::Invalid) {
        return std::make_unique<HistoryItem>(mimeDataList, type, image);
    }
    return HistoryItemPtr(); // Failed.
}

HistoryItemPtr HistoryItem::create(QDataStream &dataStream)
{
    using namespace Mimetypes;
    if (dataStream.atEnd()) {
        return HistoryItemPtr();
    }
    MimeDataList mimeDataList;
    dataStream >> mimeDataList;
    // We don't check if image formats other than application/x-qt-image produce
    // a null QImage because it could get really expensive with large images and
    // slow image formats. This function may be called whenever an app that did
    // a copy is closed and klipper takes responsibility for the clipboard item
    // or on startup when a previous session's clipboard history is being read.
    QImage image = value<QImage>(mimeDataList, Application::xQtImage);
    if (auto type = getType(mimeDataList); type != HistoryItemType::Invalid) {
        return std::make_unique<HistoryItem>(mimeDataList, type, image);
    }
    qCWarning(KLIPPER_LOG) << "Failed to restore history item: Could not read MIME data";
    return HistoryItemPtr();
}

HistoryItemPtr HistoryItem::create(const QString &text)
{
    if (text.isEmpty()) {
        return nullptr;
    }
    return std::make_unique<HistoryItem>(MimeDataList{{Mimetypes::Text::plain, text}}, HistoryItemType::Text);
}

HistoryItemPtr HistoryItem::create(const QImage &image)
{
    if (image.isNull()) {
        return nullptr;
    }
    return std::make_unique<HistoryItem>(MimeDataList{{Mimetypes::Application::xQtImage, image}}, HistoryItemType::Image, image);
}

HistoryItemPtr HistoryItem::create(const QUrlList &urls)
{
    if (isEmpty(urls)) {
        return nullptr;
    }
    return std::make_unique<HistoryItem>(MimeDataList{{Mimetypes::Text::uriList, QVariant::fromValue(urls)}}, HistoryItemType::Url);
}

#include "moc_historyitem.cpp"
