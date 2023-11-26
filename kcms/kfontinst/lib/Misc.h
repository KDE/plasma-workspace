#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "KfiConstants.h"
#include <QDataStream>
#include <QFile>
#include <QUrl>

#include "kfontinst_export.h"

class QTextStream;
class QByteArray;

namespace KFI
{
namespace Misc
{
enum EConstants {
    FILE_PERMS = 0644,
    DIR_PERMS = 0755,
};

struct TFont {
    TFont(const QString &f = QString(), quint32 s = KFI_NO_STYLE_INFO)
        : family(f)
        , styleInfo(s)
    {
    }

    bool operator==(const TFont &o) const
    {
        return o.styleInfo == styleInfo && o.family == family;
    }

    QString family;
    quint32 styleInfo;
};

extern KFONTINST_EXPORT QString prettyUrl(const QUrl &url);
inline KFONTINST_EXPORT bool isHidden(const QString &f)
{
    return f.startsWith(QChar('.'));
}
inline KFONTINST_EXPORT bool isHidden(const QUrl &url)
{
    return isHidden(url.fileName());
}
extern KFONTINST_EXPORT bool check(const QString &path, bool file, bool checkW = false);
inline KFONTINST_EXPORT bool fExists(const QString &p)
{
    return check(p, true, false);
}
inline KFONTINST_EXPORT bool dExists(const QString &p)
{
    return check(p, false, false);
}
inline KFONTINST_EXPORT bool fWritable(const QString &p)
{
    return check(p, true, true);
}
inline KFONTINST_EXPORT bool dWritable(const QString &p)
{
    return check(p, false, true);
}
extern KFONTINST_EXPORT QString linkedTo(const QString &i);
extern KFONTINST_EXPORT QString dirSyntax(const QString &d); // Has trailing slash:  /file/path/
extern KFONTINST_EXPORT QString fileSyntax(const QString &f);
extern KFONTINST_EXPORT QString getDir(const QString &f);
extern KFONTINST_EXPORT QString getFile(const QString &f);
extern KFONTINST_EXPORT bool createDir(const QString &dir);
extern KFONTINST_EXPORT void setFilePerms(const QByteArray &f);
inline KFONTINST_EXPORT void setFilePerms(const QString &f)
{
    setFilePerms(QFile::encodeName(f));
}
extern KFONTINST_EXPORT QString changeExt(const QString &f, const QString &newExt);
extern KFONTINST_EXPORT bool doCmd(const QString &cmd, const QString &p1 = QString(), const QString &p2 = QString(), const QString &p3 = QString());
inline KFONTINST_EXPORT bool root()
{
    return 0 == getuid();
}
extern KFONTINST_EXPORT void getAssociatedFiles(const QString &file, QStringList &list, bool afmAndPfm = true);
extern KFONTINST_EXPORT time_t getTimeStamp(const QString &item);
extern KFONTINST_EXPORT QString getFolder(const QString &defaultDir, const QString &root, QStringList &dirs);
extern KFONTINST_EXPORT bool checkExt(const QString &fname, const QString &ext);
extern KFONTINST_EXPORT bool isBitmap(const QString &str);
extern KFONTINST_EXPORT bool isMetrics(const QString &str);
inline KFONTINST_EXPORT bool isMetrics(const QUrl &url)
{
    return isMetrics(url.fileName());
}
inline KFONTINST_EXPORT bool isPackage(const QString &file)
{
    return file.indexOf(KFI_FONTS_PACKAGE) == (file.length() - KFI_FONTS_PACKAGE_LEN);
}
extern KFONTINST_EXPORT int getIntQueryVal(const QUrl &url, const char *key, int defVal);
extern KFONTINST_EXPORT bool printable(const QString &mime);
inline KFONTINST_EXPORT QString hide(const QString &f)
{
    return '.' != f[0] ? QChar('.') + f : f;
}
inline KFONTINST_EXPORT QString unhide(const QString &f)
{
    return '.' == f[0] ? f.mid(1) : f;
}
extern KFONTINST_EXPORT uint qHash(const TFont &key);
extern KFONTINST_EXPORT QString encodeText(const QString &str);
extern KFONTINST_EXPORT QString encodeText(const QString &str, QTextStream &s);
extern KFONTINST_EXPORT QString contractHome(QString path);
extern KFONTINST_EXPORT QString expandHome(QString path);
extern KFONTINST_EXPORT QMap<QString, QString> getFontFileMap(const QSet<QString> &files);
extern KFONTINST_EXPORT QString modifyName(const QString &fname);
inline QString getDestFolder(const QString &folder, const QString &file)
{
    return folder + file[0].toLower() + '/';
}
extern KFONTINST_EXPORT QString app(const QString &name, const char *path = nullptr);
}

}

inline KFONTINST_EXPORT QDataStream &operator<<(QDataStream &ds, const KFI::Misc::TFont &font)
{
    ds << font.family << font.styleInfo;
    return ds;
}

inline KFONTINST_EXPORT QDataStream &operator>>(QDataStream &ds, KFI::Misc::TFont &font)
{
    ds >> font.family >> font.styleInfo;
    return ds;
}
