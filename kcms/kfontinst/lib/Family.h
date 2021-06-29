#ifndef __FAMILY_H__
#define __FAMILY_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Style.h"
#include "kfontinst_export.h"
#include <QDBusArgument>
#include <QMetaType>
#include <QSet>

class QDomElement;
class QTextStream;

namespace KFI
{
class KFONTINST_EXPORT Family
{
public:
    Family(const QString &n = QString())
        : itsName(n)
    {
    }
    Family(const QDomElement &elem, bool loadStyles);

    bool operator==(const Family &o) const
    {
        return itsName == o.itsName;
    }

    void toXml(bool disabled, QTextStream &s) const;
    StyleCont::ConstIterator add(const Style &s) const
    {
        return itsStyles.insert(s);
    }
    void remove(const Style &s) const
    {
        itsStyles.remove(s);
    }
    void setStyles(const StyleCont &s) const
    {
        itsStyles = s;
    }
    const QString &name() const
    {
        return itsName;
    }
    const StyleCont styles() const
    {
        return itsStyles;
    }

private:
    QString itsName;
    mutable StyleCont itsStyles;
};

typedef QSet<Family> FamilyCont;

struct KFONTINST_EXPORT Families {
    Families(const Family &f, bool sys)
        : isSystem(sys)
    {
        items.insert(f);
    }
    Families(bool sys = false)
        : isSystem(sys)
    {
    }

    bool isSystem;
    FamilyCont items;
};

inline Q_DECL_EXPORT uint qHash(const Family &key)
{
    return qHash(key.name());
}

}

Q_DECLARE_METATYPE(KFI::Family)
Q_DECLARE_METATYPE(KFI::Families)
Q_DECLARE_METATYPE(QList<KFI::Families>)

Q_DECL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KFI::Family &obj);
Q_DECL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::Family &obj);
Q_DECL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KFI::Families &obj);
Q_DECL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::Families &obj);

#endif
