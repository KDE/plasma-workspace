/*****************************************************************

Copyright (C) 2011 Craig Drummond <craig@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include "launcherconfig.h"
#include "launcherproperties.h"
#include <KConfigDialog>
#include <KConfigGroup>
#include <KMessageBox>
#include <KLocalizedString>

#include <QtWidgets/QWhatsThis>
#include <QtCore/QUrl>
#include <kconfig.h>

namespace TaskManager
{

#define SEP "::"

LauncherConfig::LauncherConfig(KConfigDialog *parent)
    : QWidget(parent)
{
    connect(parent, SIGNAL(applyClicked()), SLOT(save()));
    connect(parent, SIGNAL(okClicked()), SLOT(save()));
    parent->addPage(this, i18n("Launcher Matching Rules"), "fork");

    ui.setupUi(this);
    ui.add->setIcon(QIcon::fromTheme("list-add"));
    ui.edit->setIcon(QIcon::fromTheme("document-edit"));
    ui.remove->setIcon(QIcon::fromTheme("list-remove"));
    ui.edit->setEnabled(false);
    ui.remove->setEnabled(false);
    connect(ui.add, SIGNAL(clicked(bool)), SLOT(add()));
    connect(ui.edit, SIGNAL(clicked(bool)), SLOT(edit()));
    connect(ui.remove, SIGNAL(clicked(bool)), SLOT(remove()));
    connect(ui.view, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
    connect(this, SIGNAL(modified()), parent, SLOT(settingsModified()));
    connect(ui.label, SIGNAL(leftClickedUrl(const QString&)), SLOT(showMoreInfo()));
    load();
}

LauncherConfig::~LauncherConfig()
{
}

void LauncherConfig::load()
{
    KConfig cfg("taskmanagerrulesrc");
    KConfigGroup grp(&cfg, "Mapping");

    foreach (const QString & key, grp.keyList()) {
        QString launcher = grp.readEntry(key, QString());

        if (launcher.isEmpty()) {
            continue;
        }

        if (launcher.endsWith(".desktop")) {
            launcher = QUrl(launcher).toDisplayString(QUrl::PrettyDecoded);
        }

        int     sepPos = key.indexOf(SEP);
        QString classClass,
                className;

        if (key.contains(SEP)) {
            classClass = key.left(sepPos);
            className = key.mid(sepPos + 2);
        } else {
            classClass = key;
        }

        new QTreeWidgetItem(ui.view, QStringList() << classClass << className << launcher);
    }

    if (ui.view->topLevelItemCount()) {
        ui.view->header()->resizeSections(QHeaderView::ResizeToContents);
    }
}

void LauncherConfig::save()
{
    QMap<QString, QString> entries;
    KConfig                cfg("taskmanagerrulesrc");
    KConfigGroup           grp(&cfg, "Mapping");

    // Go over view and create entries...
    for (int i = 0; i < ui.view->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui.view->topLevelItem(i);

        entries.insertMulti(item->text(0) + (item->text(1).isEmpty() ? QString() : (SEP + item->text(1))),
                            item->text(2));
    }

    // Remove old values...
    QSet<QString>          newKeys = entries.keys().toSet(),
                           oldKeys = grp.keyList().toSet(),
                           removedKeys = oldKeys.subtract(newKeys);

    foreach (const QString & key, removedKeys) {
        grp.deleteEntry(key);
    }

    // Store new values...
    QMapIterator<QString, QString> i(entries);
    while (i.hasNext()) {
        i.next();
        grp.writeEntry(i.key(), i.value());
    }
}

void LauncherConfig::add()
{
    LauncherProperties *prop = new LauncherProperties(this);
    connect(prop, SIGNAL(properties(const QString &, const QString &, const QString &)), SLOT(addWithProperties(const QString &, const QString &, const QString &)));
    prop->run();
}

void LauncherConfig::addWithProperties(const QString &classClass, const QString &className, const QString &launcher)
{
    for (int i = 0; i < ui.view->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui.view->topLevelItem(i);

        if (item->text(0) == classClass && item->text(1) == className) {
            KMessageBox::error(this, i18n("A launcher is already defined for %1",
                                          classClass + (className.isEmpty() ? QString() : (QChar(' ') + className))));
            return;
        }
    }
    new QTreeWidgetItem(ui.view, QStringList() << classClass << className << launcher);
}

void LauncherConfig::edit()
{
    QList<QTreeWidgetItem*> items = ui.view->selectedItems();

    if (1 == items.count()) {
        QTreeWidgetItem *item = items.at(0);
        LauncherProperties *prop = new LauncherProperties(this);
        connect(prop, SIGNAL(properties(const QString &, const QString &, const QString &)), SLOT(setProperties(const QString &, const QString &, const QString &)));
        prop->run(item->text(0), item->text(1), item->text(2));
    }
}

void LauncherConfig::setProperties(const QString &classClass, const QString &className, const QString &launcher)
{
    QList<QTreeWidgetItem*> items = ui.view->selectedItems();

    if (1 == items.count()) {
        QTreeWidgetItem *item = items.at(0);
        if (item->text(0) != classClass ||
                item->text(1) != className ||
                item->text(2) != launcher) {
            item->setText(0, classClass);
            item->setText(1, className);
            item->setText(2, launcher);
            emit modified();
        }
    }
}

void LauncherConfig::remove()
{
    QList<QTreeWidgetItem*> items = ui.view->selectedItems();

    if (1 == items.count()) {
        delete items.at(0);
        emit modified();
    }
}

void LauncherConfig::selectionChanged()
{
    bool enable = 1 == ui.view->selectedItems().count();
    ui.edit->setEnabled(enable);
    ui.remove->setEnabled(enable);
}

void LauncherConfig::showMoreInfo()
{
    QWhatsThis::showText(ui.label->mapToGlobal(QPoint(0, 0)),
                         i18n("To associate an application with a launcher, the task manager "
                              "reads the application's window class and name. These are then "
                              "used to look up the launcher details of an installed application. "
                              "This attempts to match these against the application's \'Name\'. "
                              "This can sometimes fail. The list above allows you to manually "
                              "set the class+name to launcher/name mapping."), ui.label);
}

}

#include "launcherconfig.moc"
