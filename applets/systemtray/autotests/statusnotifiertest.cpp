/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <iostream>
#include <string>

#include <QApplication>
#include <QMenu>
#include <QPixmap>

#include <KStatusNotifierItem>

using namespace Qt::StringLiterals;

class StatusNotifierTest : public QObject
{
    Q_OBJECT

public:
    explicit StatusNotifierTest(QObject *parent = nullptr);
    ~StatusNotifierTest() override;

private Q_SLOTS:
    void actionTriggered();
    void activateRequested(bool active, const QPoint &pos);
    void scrollRequested(int delta, Qt::Orientation orientation);
    void secondaryActivateRequested(const QPoint &pos);

private:
    KStatusNotifierItem m_systemNotifier;
};

StatusNotifierTest::StatusNotifierTest(QObject *parent)
    : QObject(parent)
{
    QGuiApplication::setDesktopFileName(u"org.kde.statusnotifiertest"_s);
    // m_systemNotifier.setCategory(KStatusNotifierItem::SystemServices);
    // m_systemNotifier.setCategory(KStatusNotifierItem::Hardware);
    m_systemNotifier.setCategory(KStatusNotifierItem::Communications);
    QPixmap icon(16, 16);
    icon.fill(Qt::red);
    m_systemNotifier.setIconByPixmap(icon);

    m_systemNotifier.setStatus(KStatusNotifierItem::Active);
    m_systemNotifier.setToolTipTitle(u"StatusNotifierTest"_s);
    m_systemNotifier.setTitle(u"StatusNotifierTest"_s);
    m_systemNotifier.setToolTipSubTitle(u"Some explanation from the beach."_s);

    auto menu = new QMenu;
    icon.fill(QColor(0, 255, 0)); // green
    QAction *action = menu->addAction(QIcon(icon), u"NeedsAttention"_s);
    connect(action, &QAction::triggered, this, &StatusNotifierTest::actionTriggered);
    icon.fill(QColor(255, 85, 255)); // purple
    action = menu->addAction(QIcon(icon), u"Active"_s);
    connect(action, &QAction::triggered, this, &StatusNotifierTest::actionTriggered);

    auto subMenu = new QMenu();
    subMenu->setTitle(u"Sub Menu"_s);
    icon.fill(QColor(85, 0, 255));
    subMenu->setIcon(QIcon(icon));
    icon.fill(QColor(255, 255, 0)); // yellow
    QAction *subaction = subMenu->addAction(QIcon(icon), u"Passive"_s);
    connect(subaction, &QAction::triggered, this, &StatusNotifierTest::actionTriggered);
    menu->addMenu(subMenu);

    m_systemNotifier.setContextMenu(menu);

    connect(&m_systemNotifier, &KStatusNotifierItem::activateRequested, this, &StatusNotifierTest::activateRequested);
    connect(&m_systemNotifier, &KStatusNotifierItem::secondaryActivateRequested, this, &StatusNotifierTest::secondaryActivateRequested);
    connect(&m_systemNotifier, &KStatusNotifierItem::scrollRequested, this, &StatusNotifierTest::scrollRequested);
}

StatusNotifierTest::~StatusNotifierTest()
{
}

void StatusNotifierTest::actionTriggered()
{
    const QString text = static_cast<QAction *>(sender())->text();
    std::cout << text.toStdString() << std::endl;

    if (text == u"NeedsAttention") {
        m_systemNotifier.setStatus(KStatusNotifierItem::ItemStatus::NeedsAttention);
        QPixmap icon(16, 16);
        icon.fill(Qt::blue);
        m_systemNotifier.setIconByPixmap(icon);
    } else if (text == u"Active") {
        m_systemNotifier.setStatus(KStatusNotifierItem::ItemStatus::Active);
        QPixmap icon(16, 16);
        icon.fill(Qt::red);
        m_systemNotifier.setIconByPixmap(icon);
    } else if (text == u"Passive") {
        m_systemNotifier.setStatus(KStatusNotifierItem::ItemStatus::Passive);
    } else [[unlikely]] {
        Q_UNREACHABLE();
    }
}

void StatusNotifierTest::activateRequested(bool active, const QPoint &pos)
{
    Q_UNUSED(active);
    Q_UNUSED(pos);
    std::cout << "Activated" << std::endl;
}

void StatusNotifierTest::secondaryActivateRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    std::cout << "SecondaryActivated" << std::endl;
}

void StatusNotifierTest::scrollRequested(int delta, Qt::Orientation orientation)
{
    std::string msg("Scrolled by ");
    msg += std::to_string(delta);
    msg += orientation == Qt::Horizontal ? " Horizontally" : " Vertically";
    std::cout << msg << std::endl;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    StatusNotifierTest test;
    return app.exec();
}

#include "statusnotifiertest.moc"
