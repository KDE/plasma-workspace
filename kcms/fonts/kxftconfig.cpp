/*
    SPDX-FileCopyrightText: 2002 Craig Drummond <craig@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kxftconfig.h"
#ifdef HAVE_FONTCONFIG

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <private/qtx11extras_p.h>

#include <KLocalizedString>

#include <fontconfig/fontconfig.h>

using namespace std;
using namespace Qt::StringLiterals;

static int point2Pixel(double point)
{
    return (int)(((point * QX11Info::appDpiY()) / 72.0) + 0.5);
}

static int pixel2Point(double pixel)
{
    return (int)(((pixel * 72.0) / (double)QX11Info::appDpiY()) + 0.5);
}

static bool equal(double d1, double d2)
{
    return (fabs(d1 - d2) < 0.0001);
}

static QString dirSyntax(const QString &d)
{
    if (d.isNull()) {
        return d;
    }

    QString ds(d);
    ds.replace(QLatin1String("//"), QLatin1String("/"));
    if (!ds.endsWith(QLatin1Char('/'))) {
        ds += QLatin1Char('/');
    }

    return ds;
}

inline bool fExists(const QString &p)
{
    return QFileInfo(p).isFile();
}

inline bool dWritable(const QString &p)
{
    QFileInfo info(p);
    return info.isDir() && info.isWritable();
}

static QString getDir(const QString &path)
{
    QString str(path);

    const int slashPos = str.lastIndexOf(QLatin1Char('/'));
    if (slashPos != -1) {
        str.truncate(slashPos + 1);
    }

    return dirSyntax(str);
}

static QDateTime getTimeStamp(const QString &item)
{
    return QFileInfo(item).lastModified();
}

static QString getEntry(QDomElement element, const char *type, unsigned int numAttributes, ...)
{
    if (numAttributes == uint(element.attributes().length())) {
        va_list args;
        unsigned int arg;
        bool ok = true;

        va_start(args, numAttributes);

        for (arg = 0; arg < numAttributes && ok; ++arg) {
            const char *attr = va_arg(args, const char *);
            const char *val = va_arg(args, const char *);

            if (!attr || !val || QLatin1String(val) != element.attribute(QString::fromLocal8Bit(attr))) {
                ok = false;
            }
        }

        va_end(args);

        if (ok) {
            QDomNode n = element.firstChild();

            if (!n.isNull()) {
                QDomElement e = n.toElement();

                if (!e.isNull() && QLatin1String(type) == e.tagName()) {
                    return e.text();
                }
            }
        }
    }

    return QString();
}

static KXftConfig::SubPixel::Type strToType(QStringView str)
{
    if (str == u"rgb") {
        return KXftConfig::SubPixel::Rgb;
    } else if (str == u"bgr") {
        return KXftConfig::SubPixel::Bgr;
    } else if (str == u"vrgb") {
        return KXftConfig::SubPixel::Vrgb;
    } else if (str == u"vbgr") {
        return KXftConfig::SubPixel::Vbgr;
    } else if (str == u"none") {
        return KXftConfig::SubPixel::None;
    } else {
        return KXftConfig::SubPixel::NotSet;
    }
}

static KXftConfig::Hint::Style strToStyle(QStringView str)
{
    if (str == u"hintslight") {
        return KXftConfig::Hint::Slight;
    } else if (str == u"hintmedium") {
        return KXftConfig::Hint::Medium;
    } else if (str == u"hintfull") {
        return KXftConfig::Hint::Full;
    } else {
        return KXftConfig::Hint::None;
    }
}

KXftConfig::KXftConfig(const QString &path)
    : m_doc(u"fontconfig"_s)
    , m_file(path.isEmpty() ? getConfigFile() : path)
{
    qDebug() << "Using fontconfig file:" << m_file;
    reset();
}

KXftConfig::~KXftConfig()
{
}

//
// Obtain location of config file to use.
QString KXftConfig::getConfigFile()
{
    FcStrList *list = FcConfigGetConfigFiles(FcConfigGetCurrent());
    QStringList localFiles;
    FcChar8 *file;
    QString home(dirSyntax(QDir::homePath()));

    m_globalFiles.clear();

    while ((file = FcStrListNext(list))) {
        QString f(QString::fromLocal8Bit((const char *)file));

        if (fExists(f) && 0 == f.indexOf(home)) {
            localFiles.append(f);
        } else {
            m_globalFiles.append(f);
        }
    }
    FcStrListDone(list);

    //
    // Go through list of localFiles, looking for the preferred one...
    if (!localFiles.isEmpty()) {
        for (const QString &file : std::as_const(localFiles)) {
            if (file.endsWith(QLatin1String("/fonts.conf")) || file.endsWith(QLatin1String("/.fonts.conf"))) {
                return file;
            }
        }
        return localFiles.front(); // Just return the 1st one...
    } else { // Hmmm... no known localFiles?
        if (FcGetVersion() >= 21000) {
            const QString targetPath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1Char('/') + QLatin1String("fontconfig"));
            QDir target(targetPath);
            if (!target.exists()) {
                target.mkpath(targetPath);
            }
            return targetPath + QLatin1String("/fonts.conf");
        } else {
            return home + QLatin1String("/.fonts.conf");
        }
    }
}

bool KXftConfig::reset()
{
    m_madeChanges = false;
    m_hint.reset();
    m_hinting.reset();
    m_excludeRange.reset();
    m_excludePixelRange.reset();
    m_subPixel.reset();
    m_antiAliasing.reset();
    m_antiAliasingHasLocalConfig = false;
    m_subPixelHasLocalConfig = false;
    m_hintHasLocalConfig = false;

    bool ok = false;
    std::for_each(m_globalFiles.cbegin(), m_globalFiles.cend(), [this, &ok](const QString &file) {
        ok |= parseConfigFile(file);
    });

    AntiAliasing globalAntialiasing;
    globalAntialiasing.state = m_antiAliasing.state;
    SubPixel globalSubPixel;
    globalSubPixel.type = m_subPixel.type;
    Hint globalHint;
    globalHint.style = m_hint.style;
    Exclude globalExcludeRange;
    globalExcludeRange.from = m_excludeRange.from;
    globalExcludeRange.to = m_excludePixelRange.to;
    Exclude globalExcludePixelRange;
    globalExcludePixelRange.from = m_excludePixelRange.from;
    globalExcludePixelRange.to = m_excludePixelRange.to;
    Hinting globalHinting;
    globalHinting.set = m_hinting.set;

    m_antiAliasing.reset();
    m_subPixel.reset();
    m_hint.reset();
    m_hinting.reset();
    m_excludeRange.reset();
    m_excludePixelRange.reset();

    ok |= parseConfigFile(m_file);

    if (m_antiAliasing.node.isNull()) {
        m_antiAliasing = globalAntialiasing;
    } else {
        m_antiAliasingHasLocalConfig = true;
    }

    if (m_subPixel.node.isNull()) {
        m_subPixel = globalSubPixel;
    } else {
        m_subPixelHasLocalConfig = true;
    }

    if (m_hint.node.isNull()) {
        m_hint = globalHint;
    } else {
        m_hintHasLocalConfig = true;
    }

    if (m_hinting.node.isNull()) {
        m_hinting = globalHinting;
    }
    if (m_excludeRange.node.isNull()) {
        m_excludeRange = globalExcludeRange;
    }
    if (m_excludePixelRange.node.isNull()) {
        m_excludePixelRange = globalExcludePixelRange;
    }

    return ok;
}

bool KXftConfig::apply()
{
    bool ok = true;

    if (m_madeChanges) {
        //
        // Check if file has been written since we last read it. If it has, then re-read and add any
        // of our changes...
        if (fExists(m_file) && getTimeStamp(m_file) != m_time) {
            KXftConfig newConfig;

            newConfig.setExcludeRange(m_excludeRange.from, m_excludeRange.to);
            newConfig.setSubPixelType(m_subPixel.type);
            newConfig.setHintStyle(m_hint.style);
            newConfig.setAntiAliasing(m_antiAliasing.state);

            ok = newConfig.changed() ? newConfig.apply() : true;
            if (ok) {
                reset();
            } else {
                m_time = getTimeStamp(m_file);
            }
        } else {
            // Ensure these are always equal...
            m_excludePixelRange.from = (int)point2Pixel(m_excludeRange.from);
            m_excludePixelRange.to = (int)point2Pixel(m_excludeRange.to);

            FcAtomic *atomic = FcAtomicCreate((const unsigned char *)(QFile::encodeName(m_file).data()));

            ok = false;
            if (atomic) {
                if (FcAtomicLock(atomic)) {
                    FILE *f = fopen((char *)FcAtomicNewFile(atomic), "w");

                    if (f) {
                        applySubPixelType();
                        applyHintStyle();
                        applyAntiAliasing();
                        applyExcludeRange(false);
                        applyExcludeRange(true);

                        //
                        // Check document syntax...
                        static constexpr QStringView qtXmlHeader = u"<?xml version = '1.0'?>";
                        static constexpr QStringView xmlHeader = u"<?xml version=\"1.0\"?>";
                        static constexpr QStringView qtDocTypeLine = u"<!DOCTYPE fontconfig>";
                        static constexpr QStringView docTypeLine =
                            u"<!DOCTYPE fontconfig SYSTEM "
                            "\"fonts.dtd\">";

                        QString str(m_doc.toString());
                        qsizetype idx;

                        if (0 != str.indexOf(u"<?xml")) {
                            str.insert(0, xmlHeader);
                        } else if (0 == str.indexOf(qtXmlHeader)) {
                            str.replace(qsizetype(0), qtXmlHeader.size(), xmlHeader.constData(), xmlHeader.size());
                        }

                        if (-1 != (idx = str.indexOf(qtDocTypeLine))) {
                            str.replace(idx, qtDocTypeLine.size(), docTypeLine.constData(), docTypeLine.size());
                        }

                        //
                        // Write to file...
                        fputs(str.toLocal8Bit().constData(), f);
                        fclose(f);

                        if (FcAtomicReplaceOrig(atomic)) {
                            ok = true;
                            reset(); // Re-read contents..
                        } else {
                            FcAtomicDeleteNew(atomic);
                        }
                    }
                    FcAtomicUnlock(atomic);
                }
                FcAtomicDestroy(atomic);
            }
        }
    }

    return ok;
}

bool KXftConfig::subPixelTypeHasLocalConfig() const
{
    return m_subPixelHasLocalConfig;
}

bool KXftConfig::getSubPixelType(SubPixel::Type &type)
{
    type = m_subPixel.type;
    return SubPixel::None != m_subPixel.type;
}

void KXftConfig::setSubPixelType(SubPixel::Type type)
{
    if (type != m_subPixel.type) {
        m_subPixel.type = type;
        m_madeChanges = true;
    }
}

bool KXftConfig::hintStyleHasLocalConfig() const
{
    return m_hintHasLocalConfig;
}

bool KXftConfig::getHintStyle(Hint::Style &style)
{
    if (Hint::NotSet != m_hint.style && !m_hint.toBeRemoved) {
        style = m_hint.style;
        return true;
    } else {
        return false;
    }
}

void KXftConfig::setHintStyle(Hint::Style style)
{
    if ((Hint::NotSet == style && Hint::NotSet != m_hint.style && !m_hint.toBeRemoved)
        || (Hint::NotSet != style && (style != m_hint.style || m_hint.toBeRemoved))) {
        m_hint.toBeRemoved = (Hint::NotSet == style);
        m_hint.style = style;
        m_madeChanges = true;
    }

    if (Hint::NotSet != style) {
        setHinting(Hint::None != m_hint.style);
    }
}

void KXftConfig::setHinting(bool set)
{
    if (set != m_hinting.set) {
        m_hinting.set = set;
        m_madeChanges = true;
    }
}

bool KXftConfig::getExcludeRange(double &from, double &to)
{
    if (!equal(0, m_excludeRange.from) || !equal(0, m_excludeRange.to)) {
        from = m_excludeRange.from;
        to = m_excludeRange.to;
        return true;
    } else {
        return false;
    }
}

void KXftConfig::setExcludeRange(double from, double to)
{
    double f = from < to ? from : to, t = from < to ? to : from;

    if (!equal(f, m_excludeRange.from) || !equal(t, m_excludeRange.to)) {
        m_excludeRange.from = f;
        m_excludeRange.to = t;
        m_madeChanges = true;
    }
}

QString KXftConfig::description(SubPixel::Type t)
{
    switch (t) {
    default:
    case SubPixel::NotSet:
        return i18nc("use system subpixel setting", "Vendor default");
    case SubPixel::None:
        return i18nc("no subpixel rendering", "None");
    case SubPixel::Rgb:
        return i18n("RGB");
    case SubPixel::Bgr:
        return i18n("BGR");
    case SubPixel::Vrgb:
        return i18n("Vertical RGB");
    case SubPixel::Vbgr:
        return i18n("Vertical BGR");
    }
}

QString KXftConfig::toStr(SubPixel::Type t)
{
    switch (t) {
    default:
    case SubPixel::NotSet:
        return QString();
    case SubPixel::None:
        return u"none"_s;
    case SubPixel::Rgb:
        return u"rgb"_s;
    case SubPixel::Bgr:
        return u"bgr"_s;
    case SubPixel::Vrgb:
        return u"vrgb"_s;
    case SubPixel::Vbgr:
        return u"vbgr"_s;
    }
}

QString KXftConfig::description(Hint::Style s)
{
    switch (s) {
    default:
    case Hint::NotSet:
        return i18nc("use system hinting settings", "Vendor default");
    case Hint::Medium:
        return i18nc("medium hinting", "Medium");
    case Hint::None:
        return i18nc("no hinting", "None");
    case Hint::Slight:
        return i18nc("slight hinting", "Slight");
    case Hint::Full:
        return i18nc("full hinting", "Full");
    }
}

QString KXftConfig::toStr(Hint::Style s)
{
    switch (s) {
    default:
    case Hint::NotSet:
        return QString();
    case Hint::Medium:
        return u"hintmedium"_s;
    case Hint::None:
        return u"hintnone"_s;
    case Hint::Slight:
        return u"hintslight"_s;
    case Hint::Full:
        return u"hintfull"_s;
    }
}

bool KXftConfig::parseConfigFile(const QString &filename)
{
    bool ok = false;

    QFile f(filename);

    if (f.open(QIODevice::ReadOnly)) {
        m_time = getTimeStamp(filename);
        ok = true;
        m_doc.clear();

        if (m_doc.setContent(&f)) {
            readContents();
        }
        f.close();
    } else {
        ok = !fExists(filename) && dWritable(getDir(filename));
    }

    if (m_doc.documentElement().isNull()) {
        m_doc.appendChild(m_doc.createElement(u"fontconfig"_s));
    }

    if (ok) {
        //
        // Check exclude range values - i.e. size and pixel size...
        // If "size" range is set, ensure "pixelsize" matches...
        if (!equal(0, m_excludeRange.from) || !equal(0, m_excludeRange.to)) {
            auto pFrom = (double)point2Pixel(m_excludeRange.from), pTo = (double)point2Pixel(m_excludeRange.to);

            if (!equal(pFrom, m_excludePixelRange.from) || !equal(pTo, m_excludePixelRange.to)) {
                m_excludePixelRange.from = pFrom;
                m_excludePixelRange.to = pTo;
                m_madeChanges = true;
            }
        } else if (!equal(0, m_excludePixelRange.from) || !equal(0, m_excludePixelRange.to)) {
            // "pixelsize" set, but not "size" !!!
            m_excludeRange.from = (int)pixel2Point(m_excludePixelRange.from);
            m_excludeRange.to = (int)pixel2Point(m_excludePixelRange.to);
            m_madeChanges = true;
        }
    }

    return ok;
}

void KXftConfig::readContents()
{
    for (auto n = m_doc.documentElement().firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement e = n.toElement();

        if (e.isNull()) {
            continue;
        }

        if (u"match" != e.tagName()) {
            continue;
        }

        int childNodesCount = e.childNodes().count();
        for (auto en = e.firstChild(); !en.isNull(); en = en.nextSibling()) {
            if (en.isComment()) {
                childNodesCount--;
            }
        }

        QString str;
        if (childNodesCount == 1) {
            if (u"font" != e.attribute(u"target"_s) && u"pattern" != e.attribute(u"target"_s)) {
                break;
            }
            for (auto en = e.firstChild(); !en.isNull(); en = en.nextSibling()) {
                QDomElement ene = en.toElement();
                while (ene.isComment()) {
                    ene = ene.nextSiblingElement();
                }

                if (ene.isNull() || u"edit" != ene.tagName()) {
                    continue;
                }

                if (!(str = getEntry(ene, "const", 2, "name", "rgba", "mode", "assign")).isNull()
                    || (m_subPixel.type == SubPixel::NotSet && !(str = getEntry(ene, "const", 2, "name", "rgba", "mode", "append")).isNull())) {
                    m_subPixel.node = n;
                    m_subPixel.type = strToType(str);
                } else if (!(str = getEntry(ene, "const", 2, "name", "hintstyle", "mode", "assign")).isNull()
                           || (m_hint.style == Hint::NotSet && !(str = getEntry(ene, "const", 2, "name", "hintstyle", "mode", "append")).isNull())) {
                    m_hint.node = n;
                    m_hint.style = strToStyle(str);
                } else if (!(str = getEntry(ene, "bool", 2, "name", "hinting", "mode", "assign")).isNull()) {
                    m_hinting.node = n;
                    m_hinting.set = str.compare(u"false", Qt::CaseInsensitive) != 0;
                } else if (!(str = getEntry(ene, "bool", 2, "name", "antialias", "mode", "assign")).isNull()
                           || (m_antiAliasing.state == AntiAliasing::NotSet
                               && !(str = getEntry(ene, "bool", 2, "name", "antialias", "mode", "append")).isNull())) {
                    m_antiAliasing.node = n;
                    m_antiAliasing.state = str.compare(u"false", Qt::CaseInsensitive) != 0 ? AntiAliasing::Enabled : AntiAliasing::Disabled;
                }
            }
        } else if (childNodesCount == 3) { // CPD: Is target "font" or "pattern" ????
            if (u"font" != e.attribute(u"target"_s)) {
                break;
            }

            bool foundFalse = false;
            double from = -1.0;
            double to = -1.0;
            double pixelFrom = -1.0;
            double pixelTo = -1.0;

            for (auto en = e.firstChild(); !en.isNull(); en = en.nextSibling()) {
                if (en.isComment()) {
                    continue;
                }
                auto ene = en.toElement();
                if (u"test" == ene.tagName()) {
                    // kcmfonts used to write incorrectly more or less instead of
                    // more_eq and less_eq, so read both,
                    // first the old (wrong) one then the right one
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "size", "compare", "more")).isNull()) {
                        from = str.toDouble();
                    }
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "size", "compare", "more_eq")).isNull()) {
                        from = str.toDouble();
                    }
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "size", "compare", "less")).isNull()) {
                        to = str.toDouble();
                    }
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "size", "compare", "less_eq")).isNull()) {
                        to = str.toDouble();
                    }
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "pixelsize", "compare", "more")).isNull()) {
                        pixelFrom = str.toDouble();
                    }
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "pixelsize", "compare", "more_eq")).isNull()) {
                        pixelFrom = str.toDouble();
                    }
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "pixelsize", "compare", "less")).isNull()) {
                        pixelTo = str.toDouble();
                    }
                    if (!(str = getEntry(ene, "double", 3, "qual", "any", "name", "pixelsize", "compare", "less_eq")).isNull()) {
                        pixelTo = str.toDouble();
                    }
                } else if (u"edit" == ene.tagName() && u"false" == getEntry(ene, "bool", 2, "name", "antialias", "mode", "assign")) {
                    foundFalse = true;
                }
            }

            if ((from >= 0 || to >= 0) && foundFalse) {
                m_excludeRange.from = from < to ? from : to;
                m_excludeRange.to = from < to ? to : from;
                m_excludeRange.node = n;
            } else if ((pixelFrom >= 0 || pixelTo >= 0) && foundFalse) {
                m_excludePixelRange.from = pixelFrom < pixelTo ? pixelFrom : pixelTo;
                m_excludePixelRange.to = pixelFrom < pixelTo ? pixelTo : pixelFrom;
                m_excludePixelRange.node = n;
            }
        }
    }
}

void KXftConfig::applySubPixelType()
{
    if (SubPixel::NotSet == m_subPixel.type) {
        if (!m_subPixel.node.isNull()) {
            m_doc.documentElement().removeChild(m_subPixel.node);
            m_subPixel.node.clear();
        }
    } else {
        QDomElement matchNode = m_doc.createElement(u"match"_s);
        QDomElement typeNode = m_doc.createElement(u"const"_s);
        QDomElement editNode = m_doc.createElement(u"edit"_s);
        QDomText typeText = m_doc.createTextNode(toStr(m_subPixel.type));

        matchNode.setAttribute(u"target"_s, u"font"_s);
        editNode.setAttribute(u"mode"_s, u"assign"_s);
        editNode.setAttribute(u"name"_s, u"rgba"_s);
        editNode.appendChild(typeNode);
        typeNode.appendChild(typeText);
        matchNode.appendChild(editNode);
        if (m_subPixel.node.isNull()) {
            m_doc.documentElement().appendChild(matchNode);
        } else {
            m_doc.documentElement().replaceChild(matchNode, m_subPixel.node);
        }
        m_subPixel.node = matchNode;
    }
}

void KXftConfig::applyHintStyle()
{
    applyHinting();

    if (Hint::NotSet == m_hint.style) {
        if (!m_hint.node.isNull()) {
            m_doc.documentElement().removeChild(m_hint.node);
            m_hint.node.clear();
        }
        if (!m_hinting.node.isNull()) {
            m_doc.documentElement().removeChild(m_hinting.node);
            m_hinting.node.clear();
        }
    } else {
        QDomElement matchNode = m_doc.createElement(u"match"_s), typeNode = m_doc.createElement(u"const"_s), editNode = m_doc.createElement(u"edit"_s);
        QDomText typeText = m_doc.createTextNode(toStr(m_hint.style));

        matchNode.setAttribute(u"target"_s, u"font"_s);
        editNode.setAttribute(u"mode"_s, u"assign"_s);
        editNode.setAttribute(u"name"_s, u"hintstyle"_s);
        editNode.appendChild(typeNode);
        typeNode.appendChild(typeText);
        matchNode.appendChild(editNode);
        if (m_hint.node.isNull()) {
            m_doc.documentElement().appendChild(matchNode);
        } else {
            m_doc.documentElement().replaceChild(matchNode, m_hint.node);
        }
        m_hint.node = matchNode;
    }
}

void KXftConfig::applyHinting()
{
    QDomElement matchNode = m_doc.createElement(u"match"_s), typeNode = m_doc.createElement(u"bool"_s), editNode = m_doc.createElement(u"edit"_s);
    QDomText typeText = m_doc.createTextNode(m_hinting.set ? u"true"_s : u"false"_s);

    matchNode.setAttribute(u"target"_s, u"font"_s);
    editNode.setAttribute(u"mode"_s, u"assign"_s);
    editNode.setAttribute(u"name"_s, u"hinting"_s);
    editNode.appendChild(typeNode);
    typeNode.appendChild(typeText);
    matchNode.appendChild(editNode);
    if (m_hinting.node.isNull()) {
        m_doc.documentElement().appendChild(matchNode);
    } else {
        m_doc.documentElement().replaceChild(matchNode, m_hinting.node);
    }
    m_hinting.node = matchNode;
}

void KXftConfig::applyExcludeRange(bool pixel)
{
    Exclude &range = pixel ? m_excludePixelRange : m_excludeRange;

    if (equal(range.from, 0) && equal(range.to, 0)) {
        if (!range.node.isNull()) {
            m_doc.documentElement().removeChild(range.node);
            range.node.clear();
        }
    } else {
        QString fromString, toString;

        fromString.setNum(range.from);
        toString.setNum(range.to);

        QDomElement matchNode = m_doc.createElement(u"match"_s), fromTestNode = m_doc.createElement(u"test"_s), fromNode = m_doc.createElement(u"double"_s),
                    toTestNode = m_doc.createElement(u"test"_s), toNode = m_doc.createElement(u"double"_s), editNode = m_doc.createElement(u"edit"_s),
                    boolNode = m_doc.createElement(u"bool"_s);
        QDomText fromText = m_doc.createTextNode(fromString), toText = m_doc.createTextNode(toString), boolText = m_doc.createTextNode(u"false"_s);

        matchNode.setAttribute(u"target"_s, u"font"_s); // CPD: Is target "font" or "pattern" ????
        fromTestNode.setAttribute(u"qual"_s, u"any"_s);
        fromTestNode.setAttribute(u"name"_s, pixel ? u"pixelsize"_s : u"size"_s);
        fromTestNode.setAttribute(u"compare"_s, u"more_eq"_s);
        fromTestNode.appendChild(fromNode);
        fromNode.appendChild(fromText);
        toTestNode.setAttribute(u"qual"_s, u"any"_s);
        toTestNode.setAttribute(u"name"_s, pixel ? u"pixelsize"_s : u"size"_s);
        toTestNode.setAttribute(u"compare"_s, u"less_eq"_s);
        toTestNode.appendChild(toNode);
        toNode.appendChild(toText);
        editNode.setAttribute(u"mode"_s, u"assign"_s);
        editNode.setAttribute(u"name"_s, u"antialias"_s);
        editNode.appendChild(boolNode);
        boolNode.appendChild(boolText);
        matchNode.appendChild(fromTestNode);
        matchNode.appendChild(toTestNode);
        matchNode.appendChild(editNode);

        if (!m_antiAliasing.node.isNull()) {
            m_doc.documentElement().removeChild(range.node);
        }
        if (range.node.isNull()) {
            m_doc.documentElement().appendChild(matchNode);
        } else {
            m_doc.documentElement().replaceChild(matchNode, range.node);
        }
        range.node = matchNode;
    }
}

bool KXftConfig::antiAliasingHasLocalConfig() const
{
    return m_antiAliasingHasLocalConfig;
}

KXftConfig::AntiAliasing::State KXftConfig::getAntiAliasing() const
{
    return m_antiAliasing.state;
}

void KXftConfig::setAntiAliasing(AntiAliasing::State state)
{
    if (state != m_antiAliasing.state) {
        m_antiAliasing.state = state;
        m_madeChanges = true;
    }
}

void KXftConfig::applyAntiAliasing()
{
    if (AntiAliasing::NotSet == m_antiAliasing.state) {
        if (!m_antiAliasing.node.isNull()) {
            m_doc.documentElement().removeChild(m_antiAliasing.node);
            m_antiAliasing.node.clear();
        }
    } else {
        QDomElement matchNode = m_doc.createElement(u"match"_s);
        QDomElement typeNode = m_doc.createElement(u"bool"_s);
        QDomElement editNode = m_doc.createElement(u"edit"_s);
        QDomText typeText = m_doc.createTextNode(m_antiAliasing.state == AntiAliasing::Enabled ? u"true"_s : u"false"_s);

        matchNode.setAttribute(u"target"_s, u"font"_s);
        editNode.setAttribute(u"mode"_s, u"assign"_s);
        editNode.setAttribute(u"name"_s, u"antialias"_s);
        editNode.appendChild(typeNode);
        typeNode.appendChild(typeText);
        matchNode.appendChild(editNode);
        if (!m_antiAliasing.node.isNull()) {
            m_doc.documentElement().removeChild(m_antiAliasing.node);
        }
        m_doc.documentElement().appendChild(matchNode);
        m_antiAliasing.node = matchNode;
    }
}

// KXftConfig only parses one config file, user's .fonts.conf usually.
// If that one doesn't exist, then KXftConfig doesn't know if antialiasing
// is enabled or not. So try to find out the default value from the default font.
// Maybe there's a better way *shrug*.
bool KXftConfig::aliasingEnabled()
{
    FcPattern *pattern = FcPatternCreate();
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result;
    FcPattern *f = FcFontMatch(nullptr, pattern, &result);
    FcBool antialiased = FcTrue;
    FcPatternGetBool(f, FC_ANTIALIAS, 0, &antialiased);
    FcPatternDestroy(f);
    FcPatternDestroy(pattern);
    return antialiased == FcTrue;
}

#endif
