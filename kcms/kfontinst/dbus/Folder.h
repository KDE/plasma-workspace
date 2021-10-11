#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Family.h"
#include "File.h"
#include "Misc.h"
#include "Style.h"
#include <QSet>
#include <QString>
#include <sys/time.h>

namespace KFI
{
class Folder
{
    struct CfgFile {
        bool modified();
        void updateTimeStamp();

        bool dirty;
        QString name;
        time_t timestamp;
    };

public:
    struct FlatFont : Misc::TFont {
        FlatFont(const Family &fam, const Style &style, const File &f)
            : Misc::TFont(fam.name(), style.value())
            , writingSystems(style.writingSystems())
            , scalable(style.scalable())
            , file(f)
        {
        }
        bool operator==(const FlatFont &o) const
        {
            return file.path() == o.file.path();
        }

        qulonglong writingSystems;
        bool scalable;
        File file;
    };

    struct Flat : public QSet<FlatFont> {
        Families build(bool system) const;
    };

    Folder()
    {
    }
    ~Folder();

    void init(bool system, bool systemBus);
    const QString &location() const
    {
        return m_location;
    }
    bool allowToggling() const;
    void loadDisabled();
    void saveDisabled();
    void setDisabledDirty()
    {
        m_disabledCfg.dirty = true;
    }
    bool disabledDirty() const
    {
        return m_disabledCfg.dirty;
    }
    QStringList toXml(int max = 0);
    Families list();
    bool contains(const QString &family, quint32 style);
    void add(const Family &family);
    void addModifiedDir(const QString &dir)
    {
        m_modifiedDirs.insert(dir);
    }
    void addModifiedDirs(const QSet<QString> &dirs)
    {
        m_modifiedDirs += dirs;
    }
    bool isModified() const
    {
        return !m_modifiedDirs.isEmpty();
    }
    void clearModified()
    {
        m_modifiedDirs.clear();
    }
    void configure(bool force = false);
    Flat flatten() const;
    const FamilyCont &fonts() const
    {
        return m_fonts;
    }
    FamilyCont::ConstIterator addFont(const Family &fam)
    {
        return m_fonts.insert(fam);
    }
    void removeFont(const Family &fam)
    {
        m_fonts.remove(fam);
    }
    void clearFonts()
    {
        m_fonts.clear();
    }

private:
    bool m_isSystem;
    FamilyCont m_fonts;
    CfgFile m_disabledCfg;
    QString m_location;
    QSet<QString> m_modifiedDirs;
};

inline Q_DECL_EXPORT uint qHash(const Folder::FlatFont &key)
{
    return qHash(key.file); // +qHash(key.index());
}

}
