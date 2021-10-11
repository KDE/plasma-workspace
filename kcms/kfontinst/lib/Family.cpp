/*
    SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Family.h"
#include "Misc.h"
#include "XmlStrings.h"
#include <QDebug>
#include <QDomElement>
#include <QTextStream>

namespace KFI
{
Family::Family(const QDomElement &elem, bool loadStyles)
{
    if (elem.hasAttribute(FAMILY_ATTR)) {
        m_name = elem.attribute(FAMILY_ATTR);
    }
    if (elem.hasAttribute(NAME_ATTR)) {
        m_name = elem.attribute(NAME_ATTR);
    }
    if (loadStyles) {
        for (QDomNode n = elem.firstChild(); !n.isNull(); n = n.nextSibling()) {
            QDomElement ent = n.toElement();

            if (FONT_TAG == ent.tagName()) {
                Style style(ent, loadStyles);

                if (!style.files().isEmpty()) {
                    m_styles.insert(style);
                }
            }
        }
    }
}

void Family::toXml(bool disabled, QTextStream &s) const
{
    QString family(KFI::Misc::encodeText(m_name, s));
    QStringList entries;
    StyleCont::ConstIterator it(m_styles.begin()), end(m_styles.end());

    for (; it != end; ++it) {
        QString entry((*it).toXml(disabled, disabled ? family : QString(), s));

        if (!entry.isEmpty()) {
            entries.append(entry);
        }
    }

    if (entries.count() > 0) {
        if (!disabled) {
            s << " <" FAMILY_TAG " " NAME_ATTR "=\"" << KFI::Misc::encodeText(m_name, s) << "\">\n";
        }

        QStringList::ConstIterator it(entries.begin()), end(entries.end());

        for (; it != end; ++it) {
            s << *it << Qt::endl;
        }

        if (!disabled) {
            s << " </" FAMILY_TAG ">" << Qt::endl;
        }
    }
}

}

QDBusArgument &operator<<(QDBusArgument &argument, const KFI::Family &obj)
{
    argument.beginStructure();
    argument << obj.name();

    argument.beginArray(qMetaTypeId<KFI::Style>());
    KFI::StyleCont::ConstIterator it(obj.styles().begin()), end(obj.styles().end());
    for (; it != end; ++it) {
        argument << *it;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::Family &obj)
{
    QString name;
    argument.beginStructure();
    argument >> name;
    obj = KFI::Family(name);
    argument.beginArray();
    while (!argument.atEnd()) {
        KFI::Style st;
        argument >> st;
        obj.add(st);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const KFI::Families &obj)
{
    argument.beginStructure();
    argument << obj.isSystem;

    argument.beginArray(qMetaTypeId<KFI::Family>());
    KFI::FamilyCont::ConstIterator it(obj.items.begin()), end(obj.items.end());

    for (; it != end; ++it) {
        argument << *it;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::Families &obj)
{
    argument.beginStructure();
    argument >> obj.isSystem;
    argument.beginArray();
    while (!argument.atEnd()) {
        KFI::Family fam;
        argument >> fam;
        obj.items.insert(fam);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}
