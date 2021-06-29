/*
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QApplication>

#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QMainWindow>
#include <QMenuBar>

class MainWindow : public QMainWindow
{
public:
    MainWindow();
};

MainWindow::MainWindow()
    : QMainWindow()
{
    /*set an initial menu with the following
    Menu A
      - Item
      - Checkable Item
      - Item With Icon
      - A separator
      - Menu B
         - Item B1
     Menu C
      - DynamicItem ${timestamp}

      TopLevelItem
    */

    QAction *t;
    auto menuA = new QMenu("Menu A", this);
    menuA->addAction("Item");

    t = menuA->addAction("Checkable Item");
    t->setCheckable(true);

    t = menuA->addAction(QIcon::fromTheme("document-edit"), "Item with icon");

    menuA->addSeparator();

    auto menuB = new QMenu("Menu B", this);
    menuB->addAction("Item B1");
    menuA->addMenu(menuB);

    menuBar()->addMenu(menuA);

    auto menuC = new QMenu("Menu C", this);
    connect(menuC, &QMenu::aboutToShow, this, [menuC]() {
        menuC->clear();
        menuC->addAction("Dynamic Item " + QDateTime::currentDateTime().toString());
    });

    menuBar()->addMenu(menuC);

    menuBar()->addAction("Top Level Item");
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow mw;
    mw.show();
    return app.exec();
}
