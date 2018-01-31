#include <QtTest>
#include <QObject>
#include <QDebug>
#include "notificationsanitizer.h"

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

    //more bad formatted options. To some extent actual output doesn't matter. Garbage in, garbabe out.
    //the important thing is that it doesn't contain anything that could be parsed as the remote URL
    QTest::newRow("image remote URL no close") << "This is <img src=\"http://foo.com/boo.png>\" alt=\"cheese\">  and more text" << "This is <img alt=\"cheese\"> and more text</img>";
    QTest::newRow("image remote URL double open") << "This is <<img src=\"http://foo.com/boo.png>\"  and more text" << "This is ";
    QTest::newRow("image remote URL no entitiy close") << "This is <img src=\"http://foo.com/boo.png\"  and more text" << "This is ";
    QTest::newRow("image remote URL space in element name") << "This is < img src=\"http://foo.com/boo.png\" alt=\"cheese\" /> and more text" << "This is ";

    QTest::newRow("link") << "This is a link <a href=\"http://foo.com/boo\"/> and more text" << "This is a link <a href=\"http://foo.com/boo\"/> and more text";
}

void NotificationTest::parse()
{
    QFETCH(QString, messageIn);
    QFETCH(QString, expectedOut);

    const QString out = NotificationSanitizer::parse(messageIn);
    expectedOut = "<?xml version=\"1.0\"?><html>"  + expectedOut + "</html>\n";
    QCOMPARE(out, expectedOut);
}


QTEST_GUILESS_MAIN(NotificationTest)

#include "notifications_test.moc"
