/*
    SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "File.h"
#include "Misc.h"
#include "XmlStrings.h"
#include <QDir>
#include <QDomElement>
#include <QTextStream>

using namespace Qt::StringLiterals;

namespace KFI
{
static QString expandHome(const QString &path)
{
    QString p(path);

    return !p.isEmpty() && u'~' == p[0] ? 1 == p.length() ? QDir::homePath() : p.replace(0, 1, QDir::homePath()) : p;
}

File::File(const QDomElement &elem, bool disabled)
{
    QString path = expandHome(elem.attribute(PATH_ATTR));

    if (!disabled || Misc::fExists(path)) // Only need to check instance if we are loading the disabled file...
    {
        m_foundry = elem.attribute(FOUNDRY_ATTR);
        m_index = elem.hasAttribute(FACE_ATTR) ? elem.attribute(FACE_ATTR).toInt() : 0;
        m_path = path;
    }
}

QString File::toXml(bool disabled) const
{
    if (!disabled || Misc::isHidden(Misc::getFile(m_path))) {
        QString str(PATH_ATTR + "=\""_L1 + KFI::Misc::encodeText(KFI::Misc::contractHome(m_path)) + "\""_L1);

        if (!m_foundry.isEmpty() && QString::fromLatin1("unknown") != m_foundry) {
            str += u' ' + FOUNDRY_ATTR + "=\""_L1 + KFI::Misc::encodeText(m_foundry) + "\""_L1;
        }

        if (m_index > 0) {
            str += u' ' + FACE_ATTR + "=\""_L1 + QString::number(m_index) + "\""_L1;
        }

        return str;
    }

    return {};
}

}

QDBusArgument &operator<<(QDBusArgument &argument, const KFI::File &obj)
{
    argument.beginStructure();
    argument << obj.path() << obj.foundry() << obj.index();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KFI::File &obj)
{
    QString path, foundry;
    int index;
    argument.beginStructure();
    argument >> path >> foundry >> index;
    obj = KFI::File(path, foundry, index);
    argument.endStructure();
    return argument;
}
