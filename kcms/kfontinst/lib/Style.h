#ifndef __STYLE_H__
#define __STYLE_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "File.h"
#include "kfontinst_export.h"
#include <QDBusArgument>
#include <QMetaType>
#include <QSet>

class QDomElement;
class QTextStream;

namespace KFI
{
class KFONTINST_EXPORT Style
{
public:
    Style(quint32 v = 0, bool sc = false, qulonglong ws = 0)
        : itsValue(v)
        , itsWritingSystems(ws)
        , itsScalable(sc)
    {
    }
    Style(const QDomElement &elem, bool loadFiles);

    bool operator==(const Style &o) const
    {
        return itsValue == o.itsValue;
    }

    QString toXml(bool disabled, const QString &family, QTextStream &s) const;
    FileCont::ConstIterator add(const File &f) const
    {
        return itsFiles.insert(f);
    }
    void remove(const File &f) const
    {
        itsFiles.remove(f);
    }
    quint32 value() const
    {
        return itsValue;
    }
    void setWritingSystems(qulonglong ws) const
    {
        itsWritingSystems = ws;
    }
    qulonglong writingSystems() const
    {
        return itsWritingSystems;
    }
    const FileCont &files() const
    {
        return itsFiles;
    }
    void setScalable(bool sc = true) const
    {
        itsScalable = sc;
    }
    bool scalable() const
    {
        return itsScalable;
    }
    void clearFiles() const
    {
        itsFiles.clear();
    }
    void setFiles(const FileCont &f) const
    {
        itsFiles = f;
    }
    void addFiles(const FileCont &f) const
    {
        itsFiles += f;
    }
    void removeFiles(const FileCont &f) const
    {
        itsFiles -= f;
    }

private:
    quint32 itsValue;
    mutable qulonglong itsWritingSystems;
    mutable bool itsScalable;
    mutable FileCont itsFiles;
};

typedef QSet<Style> StyleCont;

inline Q_DECL_EXPORT uint qHash(const Style &key)
{
    return key.value();
}

}

Q_DECLARE_METATYPE(KFI::Style)
Q_DECL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KFI::Style &obj);
Q_DECL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::Style &obj);

#endif
