#pragma once

/*
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
        : m_value(v)
        , m_writingSystems(ws)
        , m_scalable(sc)
    {
    }
    Style(const QDomElement &elem, bool loadFiles);

    bool operator==(const Style &o) const
    {
        return m_value == o.m_value;
    }

    QString toXml(bool disabled, const QString &family, QTextStream &s) const;
    FileCont::ConstIterator add(const File &f) const
    {
        return m_files.insert(f);
    }
    void remove(const File &f) const
    {
        m_files.remove(f);
    }
    quint32 value() const
    {
        return m_value;
    }
    void setWritingSystems(qulonglong ws) const
    {
        m_writingSystems = ws;
    }
    qulonglong writingSystems() const
    {
        return m_writingSystems;
    }
    const FileCont &files() const
    {
        return m_files;
    }
    void setScalable(bool sc = true) const
    {
        m_scalable = sc;
    }
    bool scalable() const
    {
        return m_scalable;
    }
    void clearFiles() const
    {
        m_files.clear();
    }
    void setFiles(const FileCont &f) const
    {
        m_files = f;
    }
    void addFiles(const FileCont &f) const
    {
        m_files += f;
    }
    void removeFiles(const FileCont &f) const
    {
        m_files -= f;
    }

private:
    quint32 m_value;
    mutable qulonglong m_writingSystems;
    mutable bool m_scalable;
    mutable FileCont m_files;
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
