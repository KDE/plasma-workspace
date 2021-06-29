#ifndef __FILE_H__
#define __FILE_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfontinst_export.h"
#include <QDBusArgument>
#include <QMetaType>
#include <QSet>

class QDomElement;
class QTextStream;

namespace KFI
{
class KFONTINST_EXPORT File
{
public:
    static bool equalIndex(int a, int b)
    {
        return a <= 1 && b <= 1;
    }

    File(const QString &pth = QString(), const QString &fndry = QString(), int idx = 0)
        : itsPath(pth)
        , itsFoundry(fndry)
        , itsIndex(idx)
    {
    }
    File(const QDomElement &elem, bool disabled);

    bool operator==(const File &o) const
    {
        return equalIndex(itsIndex, o.itsIndex) && itsPath == o.itsPath;
    }

    QString toXml(bool disabledOnly, QTextStream &s) const;

    const QString &path() const
    {
        return itsPath;
    }
    const QString &foundry() const
    {
        return itsFoundry;
    }
    int index() const
    {
        return itsIndex;
    }

private:
    QString itsPath, itsFoundry;
    int itsIndex;
};

typedef QSet<File> FileCont;

inline Q_DECL_EXPORT uint qHash(const File &key)
{
    return qHash(key.path()); // +qHash(key.index());
}

}

Q_DECLARE_METATYPE(KFI::File)
Q_DECL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KFI::File &obj);
Q_DECL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::File &obj);

#endif
