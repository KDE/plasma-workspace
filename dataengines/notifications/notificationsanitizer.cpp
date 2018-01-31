/*
 *   Copyright (C) 2017 David Edmundson <davidedmundson@kde.org>
 *
 * This program is free software you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "notificationsanitizer.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QRegularExpression>
#include <QDebug>
#include <QUrl>

QString NotificationSanitizer::parse(const QString &text)
{
    // replace all \ns with <br/>
    QString t = text;

    t.replace(QLatin1String("\n"), QStringLiteral("<br/>"));
    // Now remove all inner whitespace (\ns are already <br/>s)
    t = t.simplified();
    // Finally, check if we don't have multiple <br/>s following,
    // can happen for example when "\n       \n" is sent, this replaces
    // all <br/>s in succsession with just one
    t.replace(QRegularExpression(QStringLiteral("<br/>\\s*<br/>(\\s|<br/>)*")), QLatin1String("<br/>"));
    // This fancy RegExp escapes every occurence of & since QtQuick Text will blatantly cut off
    // text where it finds a stray ampersand.
    // Only &{apos, quot, gt, lt, amp}; as well as &#123 character references will be allowed
    t.replace(QRegularExpression(QStringLiteral("&(?!(?:apos|quot|[gl]t|amp);|#)")), QLatin1String("&amp;"));

    QXmlStreamReader r(QStringLiteral("<html>") + t + QStringLiteral("</html>"));
    QString result;
    QXmlStreamWriter out(&result);

    const QVector<QString> allowedTags = {"b", "i", "u", "img", "a", "html", "br"};

    out.writeStartDocument();
    while (!r.atEnd()) {
        r.readNext();

        if (r.tokenType() == QXmlStreamReader::StartElement) {
            const QString name = r.name().toString();
            if (!allowedTags.contains(name)) {
                continue;
            }
            out.writeStartElement(name);
            if (name == QLatin1String("img")) {
                auto src = r.attributes().value("src").toString();
                auto alt = r.attributes().value("alt").toString();

                const QUrl url(src);
                if (url.isLocalFile()) {
                    out.writeAttribute(QStringLiteral("src"), src);
                } else {
                    //image denied for security reasons! Do not copy the image src here!
                }

                out.writeAttribute(QStringLiteral("alt"), alt);
            }
            if (name == QLatin1String("a")) {
                out.writeAttribute(QStringLiteral("href"), r.attributes().value("href").toString());
            }
        }

        if (r.tokenType() == QXmlStreamReader::EndElement) {
            const QString name = r.name().toString();
            if (!allowedTags.contains(name)) {
                continue;
            }
            out.writeEndElement();
        }

        if (r.tokenType() == QXmlStreamReader::Characters) {
            const auto text = r.text().toString();
            out.writeCharacters(text); //this auto escapes chars -> HTML entities
        }
    }
    out.writeEndDocument();

    if (r.hasError()) {
        qWarning() << "Notification to send to backend contains invalid XML: "
                      << r.errorString() << "line" << r.lineNumber()
                      << "col" << r.columnNumber();
    }

    // The Text.StyledText format handles only html3.2 stuff and &apos; is html4 stuff
    // so we need to replace it here otherwise it will not render at all.
    result = result.replace(QLatin1String("&apos;"), QChar('\''));


    return result;
}
