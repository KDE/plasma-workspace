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

namespace KFI
{
static QString expandHome(const QString &path)
{
    QString p(path);

    return !p.isEmpty() && '~' == p[0] ? 1 == p.length() ? QDir::homePath() : p.replace(0, 1, QDir::homePath()) : p;
}

File::File(const QDomElement &elem, bool disabled)
{
    QString path = expandHome(elem.attribute(PATH_ATTR));

    if (!disabled || Misc::fExists(path)) // Only need to check instance if we are loading the disabled file...
    {
        itsFoundry = elem.attribute(FOUNDRY_ATTR);
        itsIndex = elem.hasAttribute(FACE_ATTR) ? elem.attribute(FACE_ATTR).toInt() : 0;
        itsPath = path;
    }
}

QString File::toXml(bool disabled, QTextStream &s) const
{
    if (!disabled || Misc::isHidden(Misc::getFile(itsPath))) {
        QString str(PATH_ATTR "=\"" + KFI::Misc::encodeText(KFI::Misc::contractHome(itsPath), s) + "\"");

        if (!itsFoundry.isEmpty() && QString::fromLatin1("unknown") != itsFoundry) {
            str += " " FOUNDRY_ATTR "=\"" + KFI::Misc::encodeText(itsFoundry, s) + "\"";
        }

        if (itsIndex > 0) {
            str += " " FACE_ATTR "=\"" + QString::number(itsIndex) + "\"";
        }

        return str;
    }

    return QString();
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
