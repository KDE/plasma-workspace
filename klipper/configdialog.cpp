/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2008-2009 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configdialog.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWindow>

#include <KActionCollection>
#include <KConfigSkeleton>
#include <KLocalization>
#include <KShortcutsEditor>
#include <kconfigskeleton.h>
#include <kglobalaccel.h>
#include <kmessagebox.h>
#include <kmessagewidget.h>
#include <kwindowconfig.h>

#include "klipper_debug.h"

#include "actionstreewidget.h"
#include "editactiondialog.h"
#include "klipper.h"
#include "klippersettings.h"

using namespace Qt::StringLiterals;

/* static */ QLabel *ConfigDialog::createHintLabel(const QString &text, QWidget *parent)
{
    auto *hintLabel = new QLabel(text, parent);
    hintLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    hintLabel->setWordWrap(true);
    hintLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // The minimum width needs to be set so that QLabel will wrap the text
    // to fill that width.  Otherwise, it will try to adjust the text wrapping
    // width so that the text is laid out in a pleasing looking rectangle,
    // which unfortunately leaves a lot of white space below the actual text.
    // See the "tryWidth" block in QLabelPrivate::sizeForWidth().
    hintLabel->setMinimumWidth(400);

    return (hintLabel);
}

/* static */ QLabel *ConfigDialog::createHintLabel(const KConfigSkeletonItem *item, QWidget *parent)
{
    return (createHintLabel(item->whatsThis(), parent));
}

/* static */ QString ConfigDialog::manualShortcutString()
{
    const QList<QKeySequence> keys = KGlobalAccel::self()->globalShortcut(QCoreApplication::applicationName(), QStringLiteral("repeat_action"));
    return (keys.value(0).toString(QKeySequence::NativeText));
}

//////////////////////////
//  GeneralWidget       //
//////////////////////////

GeneralWidget::GeneralWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QFormLayout(this);

    // Retain clipboard history
    const KConfigSkeletonItem *item = KlipperSettings::self()->keepClipboardContentsItem();

    m_enableHistoryCb = new QCheckBox(item->label(), this);
    m_enableHistoryCb->setObjectName(QLatin1String("kcfg_KeepClipboardContents"));
    layout->addRow(i18n("Clipboard history:"), m_enableHistoryCb);

    // Clipboard history size
    item = KlipperSettings::self()->maxClipItemsItem();
    m_historySizeSb = new QSpinBox(this);
    m_historySizeSb->setObjectName(QLatin1String("kcfg_MaxClipItems"));
    KLocalization::setupSpinBoxFormatString(m_historySizeSb, ki18ncp("Number of entries", "%v entry", "%v entries"));
    layout->addRow(item->label(), m_historySizeSb);

    layout->addRow(QString(), new QLabel(this));

    // Synchronise selection and clipboard
    item = KlipperSettings::self()->syncClipboardsItem();
    m_syncClipboardsCb = new QCheckBox(item->label(), this);
    m_syncClipboardsCb->setObjectName(QLatin1String("kcfg_SyncClipboards"));
    layout->addRow(i18n("Selection and Clipboard:"), m_syncClipboardsCb);

    QLabel *hint = ConfigDialog::createHintLabel(item, this);
    layout->addRow(QString(), hint);
    connect(hint, &QLabel::linkActivated, this, [hint]() {
        QToolTip::showText(QCursor::pos(),
                           xi18nc("@info:tooltip",
                                  "When text or an area of the screen is highlighted with the mouse or keyboard, "
                                  "this is the <emphasis>selection</emphasis>. "
                                  "It can be pasted using the middle mouse button."
                                  "<nl/>"
                                  "<nl/>"
                                  "If the selection is explicitly copied using a <interface>Copy</interface> "
                                  "or <interface>Cut</interface> action, it is saved to the <emphasis>clipboard</emphasis>. "
                                  "It can be pasted using a <interface>Paste</interface> action."
                                  "<nl/>"
                                  "<nl/>"
                                  "When turned on, this option keeps the selection and the clipboard the same, "
                                  " so that any selection is immediately available to paste by any means. "
                                  "If it is turned off, the selection may still be saved in the clipboard history "
                                  "(subject to the option below), but it can only be pasted using the middle mouse button."),
                           hint);
    });

    layout->addRow(QString(), new QLabel(this));

    m_saveSelectionCb = new QCheckBox(i18n("Text selected with the pointer or keyboard"), this);
    m_saveSelectionCb->setObjectName(QLatin1String("kcfg_SaveSelection"));
    layout->addRow(i18n("Include in history:"), m_saveSelectionCb);

    QLabel *selectionHint = ConfigDialog::createHintLabel(i18n("Text copied explicitly is always saved "
                                                               "unless it is marked as a password."),
                                                          this);
    layout->addRow(QString(), selectionHint);

    m_saveImagesCb = new QCheckBox(i18n("Image data copied explicitly"), this);
    m_saveImagesCb->setObjectName(QLatin1String("kcfg_SaveImages"));
    layout->addRow(QString(), m_saveImagesCb);

    QLabel *imageHint = ConfigDialog::createHintLabel(i18n("For example, layers and selections copied in "
                                                           "image editors like Krita or GIMP."
                                                           "<br/><a href=\"1\">More about images in the clipboard.</a>"),
                                                      this);
    layout->addRow(QString(), imageHint);

    connect(imageHint, &QLabel::linkActivated, this, [imageHint]() {
        QToolTip::showText(QCursor::pos(),
                           xi18nc("@info:tooltip",
                                  "Clipboard will always allow copying image files as filenames and will show their thumbnails. "
                                  " This setting covers cases where images are stored directly as data. "
                                  " Some applications support both cases, but some only accept images as data."
                                  "<nl/>"
                                  "<nl/>"
                                  "Also note that the Spectacle screenshot tool will ignore this setting "
                                  "when configured to save screenshots to the clipboard. "
                                  "Any application that sends both text and image will also ignore it."),
                           imageHint);
    });
}

//////////////////////////
//  PopupWidget         //
//////////////////////////

PopupWidget::PopupWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QFormLayout(this);

    // Automatic popup
    const KConfigSkeletonItem *item = KlipperSettings::self()->uRLGrabberEnabledItem();
    m_enablePopupCb = new QCheckBox(item->label(), this);
    m_enablePopupCb->setObjectName(QLatin1String("kcfg_URLGrabberEnabled"));
    layout->addRow(i18n("Show action popup menu:"), m_enablePopupCb);

    // Replay from history popup
    item = KlipperSettings::self()->replayActionInHistoryItem();
    m_historyPopupCb = new QCheckBox(item->label(), this);
    m_historyPopupCb->setObjectName(QLatin1String("kcfg_ReplayActionInHistory"));
    layout->addRow(QString(), m_historyPopupCb);

    const QList<QKeySequence> keys = KGlobalAccel::self()->globalShortcut(QCoreApplication::applicationName(), QStringLiteral("repeat_action"));
    QLabel *hint = ConfigDialog::createHintLabel(xi18nc("@info",
                                                        "When text that matches an action pattern is selected or is chosen from "
                                                        "the clipboard history, automatically show the popup menu with applicable actions. "
                                                        "If the automatic menu is turned off here, or it is not shown for an excluded window, "
                                                        "then it can be shown by using the <shortcut>%1</shortcut> key shortcut.",
                                                        ConfigDialog::manualShortcutString()),
                                                 this);
    layout->addRow(QString(), hint);

    // Action popup time
    item = KlipperSettings::self()->timeoutForActionPopupsItem();
    m_actionTimeoutSb = new QSpinBox(this);
    m_actionTimeoutSb->setObjectName(QLatin1String("kcfg_TimeoutForActionPopups"));
    KLocalization::setupSpinBoxFormatString(m_actionTimeoutSb, ki18ncp("Unit of time", "%v second", "%v seconds"));
    m_actionTimeoutSb->setSpecialValueText(i18nc("No timeout", "None"));
    layout->addRow(item->label(), m_actionTimeoutSb);

    layout->addRow(QString(), new QLabel(this));

    // Remove whitespace
    item = KlipperSettings::self()->stripWhiteSpaceItem();
    m_stripWhitespaceCb = new QCheckBox(item->label(), this);
    m_stripWhitespaceCb->setObjectName(QLatin1String("kcfg_StripWhiteSpace"));
    layout->addRow(i18n("Options:"), m_stripWhitespaceCb);
    layout->addRow(QString(), ConfigDialog::createHintLabel(item, this));

    layout->addRow(QString(), new QLabel(this));
}

//////////////////////////
//  ActionsWidget       //
//////////////////////////

ActionsWidget::ActionsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QGridLayout(this);

    // MIME actions - handlers for URLs and files that Klipper provides by itself
    const KConfigSkeletonItem *item = KlipperSettings::self()->enableMagicMimeActionsItem();
    m_mimeActionsCb = new QCheckBox(item->label(), this);
    m_mimeActionsCb->setObjectName(QLatin1String("kcfg_EnableMagicMimeActions"));
    layout->addWidget(m_mimeActionsCb, 0, 0, 1, -1);
    layout->addWidget(ConfigDialog::createHintLabel(item, this), 1, 0, 1, -1);

    // Scrolling list
    m_actionsTree = new ActionsTreeWidget(this);
    m_actionsTree->setColumnCount(2);
    m_actionsTree->setHeaderLabels({i18nc("@title:column", "Match pattern and commands"), i18nc("@title:column", "Description")});

    layout->addWidget(m_actionsTree, 2, 0, 1, -1);
    layout->setRowStretch(2, 1);

    // Action buttons
    m_addActionButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add Action..."), this);
    connect(m_addActionButton, &QPushButton::clicked, this, &ActionsWidget::onAddAction);
    layout->addWidget(m_addActionButton, 3, 0);

    m_editActionButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-edit")), i18n("Edit Action..."), this);
    connect(m_editActionButton, &QPushButton::clicked, this, &ActionsWidget::onEditAction);
    layout->addWidget(m_editActionButton, 3, 1);
    layout->setColumnStretch(2, 1);

    m_deleteActionButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Delete Action"), this);
    connect(m_deleteActionButton, &QPushButton::clicked, this, &ActionsWidget::onDeleteAction);
    layout->addWidget(m_deleteActionButton, 3, 3);

    // Where to configure the action options
    if (KlipperSettings::actionsInfoMessageShown()) {
        auto *msg = new KMessageWidget(xi18nc("@info",
                                              "These actions appear in the popup menu "
                                              "which can be configured on the <interface>Action Menu</interface> page."),
                                       this);
        msg->setMessageType(KMessageWidget::Information);
        msg->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
        msg->setWordWrap(true);
        msg->setCloseButtonVisible(true);

        connect(msg, &KMessageWidget::hideAnimationFinished, this, []() {
            KlipperSettings::setActionsInfoMessageShown(false);
        });
        layout->addWidget(msg, 4, 0, 1, -1);
    }

    // Add some vertical space between our buttons and the dialogue buttons
    layout->setRowMinimumHeight(5, 16);

    KConfigGroup oldConfig = KSharedConfig::openConfig()->group(u"ActionsWidget"_s);
    KConfigGroup state = KSharedConfig::openStateConfig()->group(u"klipper"_s).group(u"ActionsWidget"_s);
    oldConfig.moveValuesTo(state);
    QByteArray hdrState = state.readEntry("ColumnState", QByteArray());
    if (!hdrState.isEmpty()) {
        qCDebug(KLIPPER_LOG) << "Restoring column state";
        m_actionsTree->header()->restoreState(QByteArray::fromBase64(hdrState));
    } else {
        m_actionsTree->header()->resizeSection(0, 250);
    }

    connect(m_actionsTree, &QTreeWidget::itemSelectionChanged, this, &ActionsWidget::onSelectionChanged);
    connect(m_actionsTree, &QTreeWidget::itemDoubleClicked, this, &ActionsWidget::onEditAction);
    connect(m_actionsTree, &QTreeWidget::itemChanged, this, &ActionsWidget::onItemChanged);

    onSelectionChanged();
}

void ActionsWidget::setActionList(const ActionList &list)
{
    qDeleteAll(m_actionList);
    m_actionList.clear();

    for (const ClipAction *action : list) {
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
    m_actionsTree->clear();

    for (const ClipAction *action : m_actionList) {
        if (!action) {
            qCDebug(KLIPPER_LOG) << "action is null!";
            continue;
        }

        auto *item = new QTreeWidgetItem;
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        updateActionItem(item, action);

        m_actionsTree->addTopLevelItem(item);
    }

    // after all actions loaded, reset modified state of tree widget.
    // Needed because tree widget reacts on item changed events to tell if it is changed
    // this will ensure that apply button state will be correctly changed
    m_actionsTree->resetModifiedState();
}

void ActionsWidget::updateActionItem(QTreeWidgetItem *item, const ClipAction *action)
{
    if (!item || !action) {
        qCDebug(KLIPPER_LOG) << "null pointer passed to function, nothing done";
        return;
    }

    // clear children if any
    item->takeChildren();
    item->setText(0, action->actionRegexPattern());
    item->setText(1, action->description());

    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
    // There is no need to explicitly set the CheckState of the action item,
    // because with the ItemIsAutoTristate flag it is completely determined
    // by the state of the child command items.

    for (const ClipCommand &command : action->commands()) {
        QStringList cmdProps;
        cmdProps << command.command << command.description;
        auto *child = new QTreeWidgetItem(item, cmdProps);
        child->setIcon(0, QIcon::fromTheme(command.icon.isEmpty() ? QStringLiteral("system-run") : command.icon));
        child->setFlags(child->flags() | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
        child->setCheckState(0, (command.isEnabled ? Qt::Checked : Qt::Unchecked));
    }
}

ActionList ActionsWidget::actionList() const
{
    // return a copy of our action list
    ActionList list;
    for (const ClipAction *action : m_actionList) {
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
    m_actionsTree->resetModifiedState();

    qCDebug(KLIPPER_LOG) << "Saving column state";
    KConfigGroup grp = KSharedConfig::openStateConfig()->group(u"klipper"_s).group(u"ActionsWidget"_s);
    grp.writeEntry("ColumnState", m_actionsTree->header()->saveState().toBase64());
}

void ActionsWidget::onSelectionChanged()
{
    const bool itemIsSelected = !m_actionsTree->selectedItems().isEmpty();
    m_editActionButton->setEnabled(itemIsSelected);
    m_deleteActionButton->setEnabled(itemIsSelected);
}

void ActionsWidget::onAddAction()
{
    EditActionDialog dlg(this);
    auto *newAct = new ClipAction;
    dlg.setAction(newAct);

    if (dlg.exec() == QDialog::Accepted) {
        m_actionList.append(newAct);

        auto *item = new QTreeWidgetItem;
        updateActionItem(item, newAct);
        m_actionsTree->addTopLevelItem(item);
        Q_EMIT widgetChanged();
    }
}

void ActionsWidget::onEditAction()
{
    QTreeWidgetItem *item = m_actionsTree->currentItem();
    if (!item) {
        return;
    }

    int commandIdx = -1;
    if (item->parent()) {
        commandIdx = item->parent()->indexOfChild(item);
        item = item->parent(); // interested in toplevel action
    }

    int idx = m_actionsTree->indexOfTopLevelItem(item);
    ClipAction *action = m_actionList.at(idx);

    if (!action) {
        qCDebug(KLIPPER_LOG) << "action is null";
        return;
    }

    EditActionDialog dlg(this);
    dlg.setAction(action, commandIdx);
    // dialog will save values into action if user hits OK
    if (dlg.exec() == QDialog::Accepted) {
        updateActionItem(item, action);
        Q_EMIT widgetChanged();
    }
}

void ActionsWidget::onDeleteAction()
{
    QTreeWidgetItem *item = m_actionsTree->currentItem();
    if (!item) {
        return;
    }

    // If the item has a parent, then it is a command (the second level
    // of the tree).  Find the complete action.
    if (item->parent()) {
        item = item->parent();
    }

    if (KMessageBox::warningContinueCancel(this,
                                           xi18nc("@info", "Delete the selected action <resource>%1</resource><nl/>and all of its commands?", item->text(1)),
                                           i18n("Confirm Delete Action"),
                                           KStandardGuiItem::del(),
                                           KStandardGuiItem::cancel(),
                                           QStringLiteral("deleteAction"),
                                           KMessageBox::Dangerous)
        == KMessageBox::Continue) {
        int idx = m_actionsTree->indexOfTopLevelItem(item);
        m_actionList.removeAt(idx);
        delete item;
        Q_EMIT widgetChanged();
    }
}

bool ActionsWidget::hasChanged() const
{
    return (m_actionsTree->actionsChanged() != -1);
}

void ActionsWidget::onItemChanged(QTreeWidgetItem *item, int /*col*/)
{
    QTreeWidgetItem *parentItem = item->parent(); // parent of the command item
    if (parentItem == nullptr) { // this is a top level action
        return;
    }

    int actionIdx = m_actionsTree->indexOfTopLevelItem(parentItem);
    ClipAction *action = m_actionList.at(actionIdx);
    int commandIdx = parentItem->indexOfChild(item);
    ClipCommand command = action->command(commandIdx);

    // Ensure that the change being made really is a check state change.
    // because this slot is also called for multiple items when they are
    // updated after the "Edit Action" dialogue is accepted.
    const bool wasEnabled = command.isEnabled;
    const bool nowEnabled = (item->checkState(0) == Qt::Checked);
    if (nowEnabled != wasEnabled) {
        command.isEnabled = nowEnabled;
        action->replaceCommand(commandIdx, command);
        Q_EMIT widgetChanged();
    }
}

//////////////////////////
//  ConfigDialog        //
//////////////////////////

ConfigDialog::ConfigDialog(QWidget *parent, KConfigSkeleton *skeleton, Klipper *klipper, KActionCollection *collection)
    : KConfigDialog(parent, QStringLiteral("preferences"), skeleton)
    , m_generalPage(new GeneralWidget(this))
    , m_popupPage(new PopupWidget(this))
    , m_actionsPage(new ActionsWidget(this))
    , m_klipper(klipper)
{
    addPage(m_generalPage, i18nc("General Config", "General"), QStringLiteral("klipper"), i18n("General Configuration"));
    addPage(m_popupPage, i18nc("Popup Menu Config", "Action Menu"), QStringLiteral("open-menu-symbolic"), i18n("Action Menu"));
    addPage(m_actionsPage, i18nc("Actions Config", "Actions Configuration"), QStringLiteral("system-run"), i18n("Actions Configuration"));

    QWidget *shortcutsPage = new QWidget(this);
    QVBoxLayout *shortcutsLayout = new QVBoxLayout(shortcutsPage);
    m_shortcutsWidget = new KShortcutsEditor(collection, shortcutsPage, KShortcutsEditor::GlobalAction);
    shortcutsLayout->addWidget(m_shortcutsWidget);

    addPage(shortcutsPage, i18nc("Shortcuts Config", "Shortcuts"), QStringLiteral("preferences-desktop-keyboard"), i18n("Shortcuts Configuration"));

    connect(m_actionsPage, &ActionsWidget::widgetChanged, this, &ConfigDialog::settingsChangedSlot);
    connect(m_shortcutsWidget, &KShortcutsEditor::keyChange, this, &ConfigDialog::settingsChangedSlot);

    // from KWindowConfig::restoreWindowSize() API documentation
    (void)winId();

    auto oldConfig = KSharedConfig::openConfig()->group(u"ConfigDialog"_s);
    auto windowStateGroup = KSharedConfig::openStateConfig()->group(u"klipper"_s).group(u"ConfigDialog"_s);
    oldConfig.moveValuesTo(windowStateGroup);

    KWindowConfig::restoreWindowSize(windowHandle(), windowStateGroup);
    resize(windowHandle()->size());
    connect(collection, &QObject::destroyed, this, &ConfigDialog::close);
    setMinimumHeight(550);
}

void ConfigDialog::updateSettings()
{
    // The user clicked "OK" or "Apply".  Save the settings from the widgets
    // to the application settings.

    if (!m_klipper) {
        qCDebug(KLIPPER_LOG) << "Klipper object is null";
        return;
    }

    m_shortcutsWidget->save();
    m_actionsPage->resetModifiedState();

    m_klipper->setURLGrabberEnabled(KlipperSettings::uRLGrabberEnabled());
    m_klipper->urlGrabber()->setActionList(m_actionsPage->actionList());
    m_klipper->saveSettings();

    KlipperSettings::self()->save();

    KConfigGroup grp = KSharedConfig::openStateConfig()->group(u"klipper"_s).group(u"ConfigDialog"_s);
    KWindowConfig::saveWindowSize(windowHandle(), grp);
}

void ConfigDialog::updateWidgets()
{
    // The dialogue is being shown.  Initialise widgets which are not
    // managed by KConfigDialogManager from the application settings.

    if (m_klipper && m_klipper->urlGrabber()) {
        m_actionsPage->setActionList(m_klipper->urlGrabber()->actionList());
    } else {
        qCDebug(KLIPPER_LOG) << "Klipper or grabber object is null";
        return;
    }
}

void ConfigDialog::updateWidgetsDefault()
{
    // The user clicked "Defaults".  Restore the default values for
    // widgets which are not managed by KConfigDialogManager.  The
    // settings of "Actions Configuration" are not reset to the default.

    m_shortcutsWidget->allDefault();
}

bool ConfigDialog::hasChanged()
{
    return (m_actionsPage->hasChanged() || m_shortcutsWidget->isModified());
}

#include "moc_configdialog.cpp"
