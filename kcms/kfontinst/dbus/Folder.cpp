/*
    SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Folder.h"
#include "FcConfig.h"
#include "KfiConstants.h"
#include "XmlStrings.h"
#include "config-fontinst.h"
#include <KShell>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QFile>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <fontconfig/fontconfig.h>

#define DISABLED_FONTS "disabledfonts"

namespace KFI
{
bool Folder::CfgFile::modified()
{
    return timestamp != Misc::getTimeStamp(name);
}

void Folder::CfgFile::updateTimeStamp()
{
    timestamp = Misc::getTimeStamp(name);
}

Folder::~Folder()
{
    saveDisabled();
}

void Folder::init(bool system, bool systemBus)
{
    m_isSystem = system;
    if (!system) {
        FcStrList *list = FcConfigGetFontDirs(FcInitLoadConfigAndFonts());
        QStringList dirs;
        FcChar8 *fcDir;

        while ((fcDir = FcStrListNext(list))) {
            dirs.append(Misc::dirSyntax((const char *)fcDir));
        }

        m_location = Misc::getFolder(Misc::dirSyntax(QDir::homePath() + "/.fonts/"), Misc::dirSyntax(QDir::homePath()), dirs);
    } else {
        m_location = KFI_DEFAULT_SYS_FONTS_FOLDER;
    }

    if ((!system && !systemBus) || (system && systemBus)) {
        FcConfig::addDir(m_location, system);
    }

    m_disabledCfg.dirty = false;
    if (m_disabledCfg.name.isEmpty()) {
        QString fileName("/" DISABLED_FONTS ".xml");

        if (system) {
            m_disabledCfg.name = QString::fromLatin1(KFI_ROOT_CFG_DIR) + fileName;
        } else {
            QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1Char('/');

            if (!Misc::dExists(path)) {
                Misc::createDir(path);
            }
            m_disabledCfg.name = path + fileName;
        }
        m_disabledCfg.timestamp = 0;
    }
}

bool Folder::allowToggling() const
{
    return Misc::fExists(m_disabledCfg.name) ? Misc::fWritable(m_disabledCfg.name) : Misc::dWritable(Misc::getDir(m_disabledCfg.name));
}

void Folder::loadDisabled()
{
    if (m_disabledCfg.dirty) {
        saveDisabled();
    }

    QFile f(m_disabledCfg.name);

    // qDebug() << m_disabledCfg.name;
    m_disabledCfg.dirty = false;
    if (f.open(QIODevice::ReadOnly)) {
        QDomDocument doc;

        if (doc.setContent(&f)) {
            for (QDomNode n = doc.documentElement().firstChild(); !n.isNull(); n = n.nextSibling()) {
                QDomElement e = n.toElement();

                if (FONT_TAG == e.tagName()) {
                    Family fam(e, false);

                    if (!fam.name().isEmpty()) {
                        Style style(e, false);

                        if (KFI_NO_STYLE_INFO != style.value()) {
                            QList<File> files;

                            if (e.hasAttribute(PATH_ATTR)) {
                                File file(e, true);

                                if (!file.path().isEmpty()) {
                                    files.append(file);
                                } else {
                                    m_disabledCfg.dirty = true;
                                }
                            } else {
                                for (QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling()) {
                                    QDomElement ent = n.toElement();

                                    if (FILE_TAG == ent.tagName()) {
                                        File file(ent, true);

                                        if (!file.path().isEmpty()) {
                                            files.append(file);
                                        } else {
                                            // qDebug() << "Set dirty from load";
                                            m_disabledCfg.dirty = true;
                                        }
                                    }
                                }
                            }

                            if (files.count() > 0) {
                                QList<File>::ConstIterator it(files.begin()), end(files.end());

                                FamilyCont::ConstIterator f(m_fonts.insert(fam));
                                StyleCont::ConstIterator s((*f).add(style));

                                for (; it != end; ++it) {
                                    (*s).add(*it);
                                }
                            }
                        }
                    }
                }
            }
        }

        f.close();
        m_disabledCfg.updateTimeStamp();
    }

    saveDisabled();
}

void Folder::saveDisabled()
{
    if (m_disabledCfg.dirty) {
        if (!m_isSystem || Misc::root()) {
            // qDebug() << m_disabledCfg.name;

            QSaveFile file;

            file.setFileName(m_disabledCfg.name);

            if (!file.open(QIODevice::WriteOnly)) {
                // qDebug() << "Exit - cant open save file";
                qApp->exit(0);
            }

            QTextStream str(&file);

            str << "<" DISABLED_FONTS ">" << Qt::endl;

            FamilyCont::ConstIterator it(m_fonts.begin()), end(m_fonts.end());

            for (; it != end; ++it) {
                (*it).toXml(true, str);
            }
            str << "</" DISABLED_FONTS ">" << Qt::endl;
            str.flush();

            if (!file.commit()) {
                // qDebug() << "Exit - cant finalize save file";
                qApp->exit(0);
            }
        }
        m_disabledCfg.updateTimeStamp();
        m_disabledCfg.dirty = false;
    }
}

QStringList Folder::toXml(int max)
{
    QStringList rv;
    FamilyCont::ConstIterator it(m_fonts.begin()), end(m_fonts.end());
    QString string;
    QTextStream str(&string);

    for (int i = 0; it != end; ++it, ++i) {
        if (0 == (i % max)) {
            if (i) {
                str << "</" FONTLIST_TAG ">" << Qt::endl;
                rv.append(string);
                string = QString();
            }
            str << "<" FONTLIST_TAG " " << SYSTEM_ATTR "=\"" << (m_isSystem ? "true" : "false") << "\">" << Qt::endl;
        }

        (*it).toXml(false, str);
    }

    if (!string.isEmpty()) {
        str << "</" FONTLIST_TAG ">" << Qt::endl;
        rv.append(string);
    }
    return rv;
}

Families Folder::list()
{
    Families fam(m_isSystem);
    FamilyCont::ConstIterator it(m_fonts.begin()), end(m_fonts.end());

    for (int i = 0; it != end; ++it, ++i) {
        fam.items.insert(*it);
    }

    return fam;
}

bool Folder::contains(const QString &family, quint32 style)
{
    FamilyCont::ConstIterator fam = m_fonts.find(Family(family));

    if (fam == m_fonts.end()) {
        return false;
    }

    StyleCont::ConstIterator st = (*fam).styles().find(Style(style));

    return st != (*fam).styles().end();
}

void Folder::add(const Family &family)
{
    FamilyCont::ConstIterator existingFamily = m_fonts.find(family);

    if (existingFamily == m_fonts.end()) {
        m_fonts.insert(family);
    } else {
        StyleCont::ConstIterator it(family.styles().begin()), end(family.styles().end());

        for (; it != end; ++it) {
            StyleCont::ConstIterator existingStyle = (*existingFamily).styles().find(*it);

            if (existingStyle == (*existingFamily).styles().end()) {
                (*existingFamily).add(*it);
            } else {
                FileCont::ConstIterator fit((*it).files().begin()), fend((*it).files().end());

                for (; fit != fend; ++fit) {
                    FileCont::ConstIterator f = (*existingStyle).files().find(*fit);

                    if (f == (*existingStyle).files().end()) {
                        (*existingStyle).add(*fit);
                    }
                }

                (*existingStyle).setWritingSystems((*existingStyle).writingSystems() | (*it).writingSystems());
                if (!(*existingStyle).scalable() && (*it).scalable()) {
                    (*existingStyle).setScalable(true);
                }
            }
        }
    }
}

void Folder::configure(bool force)
{
    // qDebug() << "EMPTY MODIFIED " << m_modifiedDirs.isEmpty();

    if (force || !m_modifiedDirs.isEmpty()) {
        saveDisabled();

        QSet<QString>::ConstIterator it(m_modifiedDirs.constBegin()), end(m_modifiedDirs.constEnd());
        QSet<QString> dirs;

        for (; it != end; ++it) {
            if (Misc::fExists((*it) + "fonts.dir")) {
                dirs.insert(KShell::quoteArg(*it));
            }
        }

        if (!dirs.isEmpty()) {
            QProcess::startDetached(QStringLiteral(KFONTINST_LIB_EXEC_DIR "/fontinst_x11"), dirs.values());
        }

        m_modifiedDirs.clear();

        // qDebug() << "RUN FC";
        Misc::doCmd("fc-cache");
        // qDebug() << "DONE";
    }
}

Folder::Flat Folder::flatten() const
{
    FamilyCont::ConstIterator fam = m_fonts.begin(), famEnd = m_fonts.end();
    Flat rv;

    for (; fam != famEnd; ++fam) {
        StyleCont::ConstIterator style((*fam).styles().begin()), styleEnd((*fam).styles().end());

        for (; style != styleEnd; ++style) {
            FileCont::ConstIterator file((*style).files().begin()), fileEnd((*style).files().end());

            for (; file != fileEnd; ++file) {
                rv.insert(FlatFont(*fam, *style, *file));
            }
        }
    }

    return rv;
}

Families Folder::Flat::build(bool system) const
{
    ConstIterator it(begin()), e(end());
    Families families(system);

    for (; it != e; ++it) {
        Family f((*it).family);
        Style s((*it).styleInfo, (*it).scalable, (*it).writingSystems);
        FamilyCont::ConstIterator fam = families.items.constFind(f);

        if (families.items.constEnd() == fam) {
            s.add((*it).file);
            f.add(s);
            families.items.insert(f);
        } else {
            StyleCont::ConstIterator st = (*fam).styles().constFind(s);

            if ((*fam).styles().constEnd() == st) {
                s.add((*it).file);
                (*fam).add(s);
            } else {
                (*st).add((*it).file);
            }
        }
    }

    return families;
}

}
