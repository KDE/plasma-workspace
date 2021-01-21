/* This file is part of the KDE project
   Copyright (C) 2000 by Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright (C) 2008-2009 by Dmitry Suzdalev <dimsuz@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "configdialog.h"

#include <KConfigSkeleton>
#include <KEditListWidget>
#include <KShortcutsEditor>
#include <kwindowconfig.h>

#include "klipper_debug.h"

#include "editactiondialog.h"
#include "klipper.h"

GeneralWidget::GeneralWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.kcfg_TimeoutForActionPopups->setSuffix(ki18np(" second", " seconds"));
    m_ui.kcfg_MaxClipItems->setSuffix(ki18np(" entry", " entries"));
}

void GeneralWidget::updateWidgets()
{
    if (m_ui.kcfg_IgnoreSelection->isChecked()) {
        m_ui.kcfg_SyncClipboards->setEnabled(false);
        m_ui.kcfg_SelectionTextOnly->setEnabled(false);
    } else if (m_ui.kcfg_SyncClipboards->isChecked()) {
        m_ui.kcfg_IgnoreSelection->setEnabled(false);
    }
}

ActionsWidget::ActionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_editActDlg(nullptr)
{
    m_ui.setupUi(this);

    m_ui.pbAddAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_ui.pbDelAction->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    m_ui.pbEditAction->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));
    m_ui.pbAdvanced->setIcon(QIcon::fromTheme(QStringLiteral("configure")));

    const KConfigGroup grp = KSharedConfig::openConfig()->group("ActionsWidget");
    QByteArray hdrState = grp.readEntry("ColumnState", QByteArray());
    if (!hdrState.isEmpty()) {
        qCDebug(KLIPPER_LOG) << "Restoring column state";
        m_ui.kcfg_ActionList->header()->restoreState(QByteArray::fromBase64(hdrState));
    } else {
        m_ui.kcfg_ActionList->header()->resizeSection(0, 250);
    }

    connect(m_ui.kcfg_ActionList, &ActionsTreeWidget::itemSelectionChanged, this, &ActionsWidget::onSelectionChanged);
    connect(m_ui.kcfg_ActionList, &ActionsTreeWidget::itemDoubleClicked, this, &ActionsWidget::onEditAction);

    connect(m_ui.pbAddAction, &QPushButton::clicked, this, &ActionsWidget::onAddAction);
    connect(m_ui.pbEditAction, &QPushButton::clicked, this, &ActionsWidget::onEditAction);
    connect(m_ui.pbDelAction, &QPushButton::clicked, this, &ActionsWidget::onDeleteAction);
    connect(m_ui.pbAdvanced, &QPushButton::clicked, this, &ActionsWidget::onAdvanced);

    onSelectionChanged();
}

void ActionsWidget::setActionList(const ActionList &list)
{
    qDeleteAll(m_actionList);
    m_actionList.clear();

    for (ClipAction *action : list) {
        if (!action) {
            qCDebug(KLIPPER_LOG) << "action is null!";
            continue;
        }

        // make a copy for us to work with from now on
        m_actionList.append(new ClipAction(*action));
    }

    updateActionListView();
}

void ActionsWidget::updateActionListView()
{
    m_ui.kcfg_ActionList->clear();

    foreach (ClipAction *action, m_actionList) {
        if (!action) {
            qCDebug(KLIPPER_LOG) << "action is null!";
            continue;
        }

        QTreeWidgetItem *item = new QTreeWidgetItem;
        updateActionItem(item, action);

        m_ui.kcfg_ActionList->addTopLevelItem(item);
    }

    // after all actions loaded, reset modified state of tree widget.
    // Needed because tree widget reacts on item changed events to tell if it is changed
    // this will ensure that apply button state will be correctly changed
    m_ui.kcfg_ActionList->resetModifiedState();
}

void ActionsWidget::updateActionItem(QTreeWidgetItem *item, ClipAction *action)
{
    if (!item || !action) {
        qCDebug(KLIPPER_LOG) << "null pointer passed to function, nothing done";
        return;
    }

    // clear children if any
    item->takeChildren();
    item->setText(0, action->actionRegexPattern());
    item->setText(1, action->description());

    foreach (const ClipCommand &command, action->commands()) {
        QStringList cmdProps;
        cmdProps << command.command << command.description;
        QTreeWidgetItem *child = new QTreeWidgetItem(item, cmdProps);
        child->setIcon(0, QIcon::fromTheme(command.icon.isEmpty() ? QStringLiteral("system-run") : command.icon));
    }
}

void ActionsWidget::setExcludedWMClasses(const QStringList &excludedWMClasses)
{
    m_exclWMClasses = excludedWMClasses;
}

QStringList ActionsWidget::excludedWMClasses() const
{
    return m_exclWMClasses;
}

ActionList ActionsWidget::actionList() const
{
    // return a copy of our action list
    ActionList list;
    foreach (ClipAction *action, m_actionList) {
        if (!action) {
            qCDebug(KLIPPER_LOG) << "action is null";
            continue;
        }
        list.append(new ClipAction(*action));
    }

    return list;
}

void ActionsWidget::resetModifiedState()
{
    m_ui.kcfg_ActionList->resetModifiedState();

    qCDebug(KLIPPER_LOG) << "Saving column state";
    KConfigGroup grp = KSharedConfig::openConfig()->group("ActionsWidget");
    grp.writeEntry("ColumnState", m_ui.kcfg_ActionList->header()->saveState().toBase64());
}

void ActionsWidget::onSelectionChanged()
{
    bool itemIsSelected = !m_ui.kcfg_ActionList->selectedItems().isEmpty();
    m_ui.pbEditAction->setEnabled(itemIsSelected);
    m_ui.pbDelAction->setEnabled(itemIsSelected);
}

void ActionsWidget::onAddAction()
{
    if (!m_editActDlg) {
        m_editActDlg = new EditActionDialog(this);
    }

    ClipAction *newAct = new ClipAction;
    m_editActDlg->setAction(newAct);
    if (m_editActDlg->exec() == QDialog::Accepted) {
        m_actionList.append(newAct);

        QTreeWidgetItem *item = new QTreeWidgetItem;
        updateActionItem(item, newAct);
        m_ui.kcfg_ActionList->addTopLevelItem(item);
    }
}

void ActionsWidget::onEditAction()
{
    if (!m_editActDlg) {
        m_editActDlg = new EditActionDialog(this);
    }

    QTreeWidgetItem *item = m_ui.kcfg_ActionList->currentItem();
    int commandIdx = -1;
    if (item) {
        if (item->parent()) {
            commandIdx = item->parent()->indexOfChild(item);
            item = item->parent(); // interested in toplevel action
        }

        int idx = m_ui.kcfg_ActionList->indexOfTopLevelItem(item);
        ClipAction *action = m_actionList.at(idx);

        if (!action) {
            qCDebug(KLIPPER_LOG) << "action is null";
            return;
        }

        m_editActDlg->setAction(action, commandIdx);
        // dialog will save values into action if user hits OK
        m_editActDlg->exec();

        updateActionItem(item, action);
    }
}

void ActionsWidget::onDeleteAction()
{
    QTreeWidgetItem *item = m_ui.kcfg_ActionList->currentItem();
    if (item && item->parent())
        item = item->parent();

    if (item) {
        int idx = m_ui.kcfg_ActionList->indexOfTopLevelItem(item);
        m_actionList.removeAt(idx);
    }

    delete item;
}

void ActionsWidget::onAdvanced()
{
    QDialog dlg(this);
    dlg.setModal(true);
    dlg.setWindowTitle(i18n("Advanced Settings"));
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    AdvancedWidget *widget = new AdvancedWidget(&dlg);
    widget->setWMClasses(m_exclWMClasses);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->addWidget(widget);
    layout->addWidget(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        m_exclWMClasses = widget->wmClasses();
    }
}

ConfigDialog::ConfigDialog(QWidget *parent, KConfigSkeleton *skeleton, const Klipper *klipper, KActionCollection *collection)
    : KConfigDialog(parent, QStringLiteral("preferences"), skeleton)
    , m_generalPage(new GeneralWidget(this))
    , m_actionsPage(new ActionsWidget(this))
    , m_klipper(klipper)
{
    addPage(m_generalPage, i18nc("General Config", "General"), QStringLiteral("klipper"), i18n("General Configuration"));
    addPage(m_actionsPage, i18nc("Actions Config", "Actions"), QStringLiteral("system-run"), i18n("Actions Configuration"));

    QWidget *w = new QWidget(this);
    m_shortcutsWidget = new KShortcutsEditor(collection, w, KShortcutsEditor::GlobalAction);
    addPage(m_shortcutsWidget, i18nc("Shortcuts Config", "Shortcuts"), QStringLiteral("preferences-desktop-keyboard"), i18n("Shortcuts Configuration"));

    const KConfigGroup grp = KSharedConfig::openConfig()->group("ConfigDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), grp);
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::updateSettings()
{
    // user clicked Ok or Apply

    if (!m_klipper) {
        qCDebug(KLIPPER_LOG) << "Klipper object is null";
        return;
    }

    m_shortcutsWidget->save();

    m_actionsPage->resetModifiedState();

    m_klipper->urlGrabber()->setActionList(m_actionsPage->actionList());
    m_klipper->urlGrabber()->setExcludedWMClasses(m_actionsPage->excludedWMClasses());
    m_klipper->saveSettings();

    KConfigGroup grp = KSharedConfig::openConfig()->group("ConfigDialog");
    KWindowConfig::saveWindowSize(windowHandle(), grp);
}

void ConfigDialog::updateWidgets()
{
    // settings were updated, update widgets

    if (m_klipper && m_klipper->urlGrabber()) {
        m_actionsPage->setActionList(m_klipper->urlGrabber()->actionList());
        m_actionsPage->setExcludedWMClasses(m_klipper->urlGrabber()->excludedWMClasses());
    } else {
        qCDebug(KLIPPER_LOG) << "Klipper or grabber object is null";
        return;
    }
    m_generalPage->updateWidgets();
}

void ConfigDialog::updateWidgetsDefault()
{
    // default widget values requested

    m_shortcutsWidget->allDefault();
}

AdvancedWidget::AdvancedWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox *groupBox = new QGroupBox(i18n("D&isable Actions for Windows of Type WM_CLASS"), this);
    groupBox->setLayout(new QVBoxLayout(groupBox));

    editListBox = new KEditListWidget(groupBox);

    editListBox->setButtons(KEditListWidget::Add | KEditListWidget::Remove);
    editListBox->setCheckAtEntering(true);

    editListBox->setWhatsThis(
        i18n("<qt>This lets you specify windows in which Klipper should "
             "not invoke \"actions\". Use<br /><br />"
             "<center><b>xprop | grep WM_CLASS</b></center><br />"
             "in a terminal to find out the WM_CLASS of a window. "
             "Next, click on the window you want to examine. The "
             "first string it outputs after the equal sign is the one "
             "you need to enter here.</qt>"));
    groupBox->layout()->addWidget(editListBox);

    mainLayout->addWidget(groupBox);

    editListBox->setFocus();
}

AdvancedWidget::~AdvancedWidget()
{
}

void AdvancedWidget::setWMClasses(const QStringList &items)
{
    editListBox->setItems(items);
}

QStringList AdvancedWidget::wmClasses() const
{
    return editListBox->items();
}
