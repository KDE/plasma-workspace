/*
    SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Style.h"
#include "Fc.h"
#include "WritingSystems.h"
#include "XmlStrings.h"
#include <QDomElement>
#include <QStringList>
#include <QTextStream>

using namespace Qt::StringLiterals;

namespace KFI
{
Style::Style(const QDomElement &elem, bool loadFiles)
{
    bool ok;
    int weight(KFI_NULL_SETTING), width(KFI_NULL_SETTING), slant(KFI_NULL_SETTING), tmp(KFI_NULL_SETTING);

    if (elem.hasAttribute(WEIGHT_ATTR)) {
        tmp = elem.attribute(WEIGHT_ATTR).toInt(&ok);
        if (ok) {
            weight = tmp;
        }
    }
    if (elem.hasAttribute(WIDTH_ATTR)) {
        tmp = elem.attribute(WIDTH_ATTR).toInt(&ok);
        if (ok) {
            width = tmp;
        }
    }

    if (elem.hasAttribute(SLANT_ATTR)) {
        tmp = elem.attribute(SLANT_ATTR).toInt(&ok);
        if (ok) {
            slant = tmp;
        }
    }

    m_scalable = !elem.hasAttribute(SCALABLE_ATTR) || elem.attribute(SCALABLE_ATTR) != "false";
    m_value = FC::createStyleVal(weight, width, slant);
    m_writingSystems = 0;

    if (elem.hasAttribute(LANGS_ATTR)) {
        m_writingSystems = WritingSystems::instance()->get(elem.attribute(LANGS_ATTR).split(LANG_SEP, Qt::SkipEmptyParts));
    }

    if (loadFiles) {
        if (elem.hasAttribute(PATH_ATTR)) {
            File file(elem, false);

            if (!file.path().isEmpty()) {
                m_files.insert(file);
            }
        } else {
            for (QDomNode n = elem.firstChild(); !n.isNull(); n = n.nextSibling()) {
                QDomElement ent = n.toElement();

                if (FILE_TAG == ent.tagName()) {
                    File file(ent, false);

                    if (!file.path().isEmpty()) {
                        m_files.insert(file);
                    }
                }
            }
        }
    }
}

QString Style::toXml(bool disabled, const QString &family) const
{
    QStringList files;
    FileCont::ConstIterator it(m_files.begin()), end(m_files.end());

    for (; it != end; ++it) {
        QString f((*it).toXml(disabled));

        if (!f.isEmpty()) {
            files.append(f);
        }
    }

    if (files.count() > 0) {
        QString str("  <"_L1 + FONT_TAG + u' ');
        int weight, width, slant;

        KFI::FC::decomposeStyleVal(m_value, weight, width, slant);

        if (!family.isEmpty()) {
            str += FAMILY_ATTR + "=\""_L1 + family + "\" "_L1;
        }
        if (KFI_NULL_SETTING != weight) {
            str += WEIGHT_ATTR + "=\""_L1 + QString::number(weight) + "\" "_L1;
        }
        if (KFI_NULL_SETTING != width) {
            str += WIDTH_ATTR + "=\""_L1 + QString::number(width) + "\" "_L1;
        }
        if (KFI_NULL_SETTING != slant) {
            str += SLANT_ATTR + "=\""_L1 + QString::number(slant) + "\" "_L1;
        }
        if (!m_scalable) {
            str += SCALABLE_ATTR + "=\"false\" "_L1;
        }

        QStringList ws(WritingSystems::instance()->getLangs(m_writingSystems));

        if (!ws.isEmpty()) {
            str += LANGS_ATTR + "=\""_L1 + ws.join(LANG_SEP) + "\" "_L1;
        }

        if (1 == files.count()) {
            str += (*files.begin()) + "/>"_L1;
        } else {
            QStringList::ConstIterator it(files.begin()), end(files.end());

            str += QStringView(u">\n");
            for (; it != end; ++it) {
                str += "   <"_L1 + FILE_TAG + u' ' + (*it) + "/>\n"_L1;
            }
            str += "  </"_L1 + FONT_TAG + ">"_L1;
        }

        return str;
    }

    return QString();
}

}

QDBusArgument &operator<<(QDBusArgument &argument, const KFI::Style &obj)
{
    argument.beginStructure();

    argument << obj.value() << obj.scalable() << obj.writingSystems();
    argument.beginArray(qMetaTypeId<KFI::File>());
    KFI::FileCont::ConstIterator it(obj.files().begin()), end(obj.files().end());
    for (; it != end; ++it) {
        argument << *it;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::Style &obj)
{
    quint32 value;
    bool scalable;
    qulonglong ws;
    argument.beginStructure();
    argument >> value >> scalable >> ws;
    obj = KFI::Style(value, scalable, ws);
    argument.beginArray();
    while (!argument.atEnd()) {
        KFI::File f;
        argument >> f;
        obj.add(f);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}
