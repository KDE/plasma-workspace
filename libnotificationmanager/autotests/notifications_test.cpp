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

#include <QtTest>
#include <QObject>
#include <QDebug>

#include "notification.h"

class NotificationTest : public QObject
{
    Q_OBJECT
public:
    NotificationTest() {}
private Q_SLOTS:
    void parse_data();
    void parse();
};

void NotificationTest::parse_data()
{
    QTest::addColumn<QString>("messageIn");
    QTest::addColumn<QString>("expectedOut");

    QTest::newRow("basic no HTML") << "I am a notification" << "I am a notification";
    QTest::newRow("whitespace") << "      I am a   notification  " << "I am a notification";

    QTest::newRow("basic html") << "I am <b>the</b> notification" << "I am <b>the</b> notification";
    QTest::newRow("nested html") << "I am <i><b>the</b></i> notification" << "I am <i><b>the</b></i> notification";

    QTest::newRow("no extra tags") << "I am <blink>the</blink> notification" << "I am the notification";
    QTest::newRow("no extra attrs") << "I am <b style=\"font-weight:20\">the</b> notification" << "I am <b>the</b> notification";

    QTest::newRow("newlines") << "I am\nthe\nnotification" << "I am<br/>the<br/>notification";
    QTest::newRow("multinewlines") << "I am\n\nthe\n\n\nnotification" << "I am<br/>the<br/>notification";

    QTest::newRow("amp") << "me&you" << "me&amp;you";
    QTest::newRow("double escape") << "foo &amp; &lt;bar&gt;" << "foo &amp; &lt;bar&gt;";

    QTest::newRow("quotes") << "&apos;foo&apos;" << "'foo'";//as label can't handle this normally valid entity

    QTest::newRow("image normal") << "This is <img src=\"file:://foo/boo.png\" alt=\"cheese\"/> and more text" << "This is <img src=\"file:://foo/boo.png\" alt=\"cheese\"/> and more text";

    //this input is technically wrong, so the output is also wrong, but QTextHtmlParser does the "right" thing
    QTest::newRow("image normal no close") << "This is <img src=\"file:://foo/boo.png\" alt=\"cheese\"> and more text" << "This is <img src=\"file:://foo/boo.png\" alt=\"cheese\"> and more text</img>";

    QTest::newRow("image remote URL") << "This is <img src=\"http://foo.com/boo.png\" alt=\"cheese\" /> and more text" << "This is <img alt=\"cheese\"/> and more text";

    //more bad formatted options. To some extent actual output doesn't matter. Garbage in, garbage out.
    //the important thing is that it doesn't contain anything that could be parsed as the remote URL
    QTest::newRow("image remote URL no close") << "This is <img src=\"http://foo.com/boo.png>\" alt=\"cheese\">  and more text" << "This is <img alt=\"cheese\"> and more text</img>";
    QTest::newRow("image remote URL double open") << "This is <<img src=\"http://foo.com/boo.png>\"  and more text" << "This is ";
    QTest::newRow("image remote URL no entity close") << "This is <img src=\"http://foo.com/boo.png\"  and more text" << "This is ";
    QTest::newRow("image remote URL space in element name") << "This is < img src=\"http://foo.com/boo.png\" alt=\"cheese\" /> and more text" << "This is ";

    QTest::newRow("link") << "This is a link <a href=\"http://foo.com/boo\"/> and more text" << "This is a link <a href=\"http://foo.com/boo\"/> and more text";
}

void NotificationTest::parse()
{
    QFETCH(QString, messageIn);
    QFETCH(QString, expectedOut);

    NotificationManager::Notification notification;
    notification.setBody(messageIn);

    expectedOut = "<?xml version=\"1.0\"?><html>"  + expectedOut + "</html>\n";

    QCOMPARE(notification.body(), expectedOut);
}

QTEST_GUILESS_MAIN(NotificationTest)

#include "notifications_test.moc"
