/*
 *   Copyright (C) 2017 David Edmundson <davidedmundson@kde.org>
 *   Copyright (C) 2020 Kai Uwe Broulik <kde@broulik.de>
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

#include <QDebug>
#include <QObject>
#include <QtTest>

#include "notification.h"
#include "notificationsmodel.h"
#include "server.h"

namespace NotificationManager
{
class NotificationTest : public QObject
{
    Q_OBJECT
public:
    NotificationTest()
    {
    }
private Q_SLOTS:
    void parse_data();
    void parse();

    void compressNotificationRemoval();
};

void NotificationTest::parse_data()
{
    QTest::addColumn<QString>("messageIn");
    QTest::addColumn<QString>("expectedOut");

    // clang-format off
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
    // clang-format on
}

void NotificationTest::parse()
{
    QFETCH(QString, messageIn);
    QFETCH(QString, expectedOut);

    NotificationManager::Notification notification;
    notification.setBody(messageIn);

    expectedOut = "<?xml version=\"1.0\"?><html>" + expectedOut + "</html>\n";

    QCOMPARE(notification.body(), expectedOut);
}

void NotificationTest::compressNotificationRemoval()
{
    const int notificationCount = 10;
    const int gapId = 4;

    auto model = NotificationsModel::createNotificationsModel();

    QSignalSpy rowsRemovedSpy(model.data(), &QAbstractItemModel::rowsRemoved);
    QVERIFY(rowsRemovedSpy.isValid());

    for (uint i = 1; i <= notificationCount; ++i) {
        Notification notification{i};
        notification.setSummary(QStringLiteral("Notification %1").arg(i));
        model->onNotificationAdded(notification);
    }

    QCOMPARE(model->rowCount(), notificationCount);

    for (uint i = 1; i <= notificationCount; ++i) {
        // Leave a gap inbetween
        if (i != gapId) {
            model->onNotificationRemoved(i, Server::CloseReason::Revoked);
        }
    }

    // We should have two ranges that we ended up removing
    QTRY_COMPARE(rowsRemovedSpy.count(), 2);

    // The fact that it emits row removal in reverse order is an implementation detail
    // We only really care that the number of rows emitted matches our expectation
    int removedCount = 0;
    for (const auto &removedEmission : rowsRemovedSpy) {
        const int from = removedEmission.at(1).toInt();
        const int to = removedEmission.at(2).toInt();
        removedCount += (to - from) + 1;
    }

    QCOMPARE(removedCount, notificationCount - 1);
    QCOMPARE(model->rowCount(), 1);

    rowsRemovedSpy.clear();

    // Removing a random non-existing notification should noop
    model->onNotificationRemoved(3, Server::CloseReason::Revoked);
    QTRY_COMPARE(rowsRemovedSpy.count(), 0);
    QCOMPARE(model->rowCount(), 1);
    rowsRemovedSpy.clear();

    // Now remove the last one
    model->onNotificationRemoved(gapId, Server::CloseReason::Revoked);
    QTRY_COMPARE(rowsRemovedSpy.count(), 1);
    QCOMPARE(rowsRemovedSpy.at(0).at(1), 0); // from
    QCOMPARE(rowsRemovedSpy.at(0).at(2), 0); // to
    QCOMPARE(model->rowCount(), 0);
}

} // namespace NotificationManager

QTEST_GUILESS_MAIN(NotificationManager::NotificationTest)

#include "notifications_test.moc"
