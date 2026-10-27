/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KSystemClipboard>

#include <QApplication>
#include <QClipboard>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMimeData>
#include <QTest>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

using namespace Qt::StringLiterals;

/**
 * A regression test for the clipboard deadlock issue fixed in KGuiAddons MR 191.
 *
 * If an application owns the data control (via KSystemClipboard) and then tries
 * to read the regular clipboard (via QClipboard), it used to deadlock because
 * the read would wait for the data control owner (this same process) to provide
 * the data. MR 191 fixed it by dispatching read events on a separate thread.
 *
 * The test requires a Wayland compositor implementing the data control protocol
 * (e.g. kwin_wayland), because KSystemClipboard falls back to QClipboard on
 * non-Wayland platforms, where the deadlock path does not exist.
 *
 * @see https://invent.kde.org/frameworks/kguiaddons/-/merge_requests/191
 */
class ClipboardDeadlockTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testSetRegularClipboard();
    void testSetSystemClipboard();

private:
    QWidget *m_window = nullptr;
    QTextEdit *m_normalView = nullptr;
    QTextEdit *m_systemView = nullptr;
};

void ClipboardDeadlockTest::initTestCase()
{
    // A small window mirroring the manual test window from kguiaddons'
    // tests/clipboardtest.cpp, showing both the regular and the system clipboard.
    m_window = new QWidget;
    m_window->setWindowTitle(u"Clipboard Test"_s);
    auto *rootLayout = new QHBoxLayout(m_window);

    {
        auto *group = new QGroupBox(u"Normal Clipboard"_s, m_window);
        rootLayout->addWidget(group);
        auto *groupLayout = new QVBoxLayout(group);
        m_normalView = new QTextEdit(group);
        m_normalView->setReadOnly(true);
        m_normalView->setMinimumHeight(200);
        auto *clipboard = QApplication::clipboard();
        QObject::connect(clipboard, &QClipboard::changed, m_normalView, [m_normalView = m_normalView, clipboard](QClipboard::Mode mode) {
            if (mode == QClipboard::Clipboard) {
                m_normalView->setText(clipboard->text());
            }
        });
        m_normalView->setText(clipboard->text());
        groupLayout->addWidget(m_normalView);
    }

    {
        auto *group = new QGroupBox(u"System Clipboard"_s, m_window);
        rootLayout->addWidget(group);
        auto *groupLayout = new QVBoxLayout(group);
        m_systemView = new QTextEdit(group);
        m_systemView->setReadOnly(true);
        m_systemView->setMinimumHeight(200);
        auto *clipboard = KSystemClipboard::instance();
        QObject::connect(clipboard, &KSystemClipboard::changed, m_systemView, [m_systemView = m_systemView, clipboard](QClipboard::Mode mode) {
            if (mode == QClipboard::Clipboard) {
                m_systemView->setText(clipboard->text(mode));
            }
        });
        m_systemView->setText(clipboard->text(QClipboard::Clipboard));
        groupLayout->addWidget(m_systemView);
    }

    m_window->resize(600, 500);
    m_window->show();
}

void ClipboardDeadlockTest::testSetRegularClipboard()
{
    const QString text = u"From Normal Clipboard"_s;
    auto *mimeData = new QMimeData;
    mimeData->setText(text);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    // Both views should observe the content without deadlocking.
    QTRY_COMPARE(m_normalView->toPlainText(), text);
    QTRY_COMPARE(m_systemView->toPlainText(), text);
}

void ClipboardDeadlockTest::testSetSystemClipboard()
{
    const QString text = u"From System Clipboard"_s;
    auto *mimeData = new QMimeData;
    mimeData->setText(text);
    KSystemClipboard::instance()->setMimeData(mimeData, QClipboard::Clipboard);

    // Both views should observe the content without deadlocking.
    QTRY_COMPARE(m_normalView->toPlainText(), text);
    QTRY_COMPARE(m_systemView->toPlainText(), text);
}

QTEST_MAIN(ClipboardDeadlockTest)

#include "clipboarddeadlocktest.moc"
