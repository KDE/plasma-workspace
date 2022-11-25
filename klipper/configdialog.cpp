/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2008-2009 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configdialog.h"

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qfontdatabase.h>
#include <qformlayout.h>
#include <qgridlayout.h>
#include <qheaderview.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qtooltip.h>
#include <qwindow.h>

#include <KConfigSkeleton>
#include <KEditListWidget>
#include <KShortcutsEditor>
#include <kconfigskeleton.h>
#include <kglobalaccel.h>
#include <kmessagebox.h>
#include <kmessagewidget.h>
#include <kpluralhandlingspinbox.h>
#include <kwindowconfig.h>

#include "klipper_debug.h"

#include "actionstreewidget.h"
#include "editactiondialog.h"
#include "klipper.h"
#include "klippersettings.h"

/* static */ QLabel *ConfigDialog::createHintLabel(const QString &text, QWidget *parent)
{
    QLabel *hintLabel = new QLabel(text, parent);
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
//  GeneralWidget	//
//////////////////////////

GeneralWidget::GeneralWidget(QWidget *parent)
    : QWidget(parent)
{
    QFormLayout *layout = new QFormLayout(this);

    // Synchronise selection and clipboard
    const KConfigSkeletonItem *item = KlipperSettings::self()->syncClipboardsItem();
    m_syncClipboardsCb = new QCheckBox(item->label(), this);
    m_syncClipboardsCb->setObjectName(QLatin1String("kcfg_SyncClipboards"));
    layout->addRow(i18n("Selection and Clipboard:"), m_syncClipboardsCb);

    QLabel *hint = ConfigDialog::createHintLabel(item, this);
    layout->addRow(QString(), hint);
    connect(hint, &QLabel::linkActivated, this, [this, hint]() {
        QToolTip::showText(QCursor::pos(),
                           xi18nc("@info:tooltip",
                                  "When text or an area of the screen is highlighted with the mouse or keyboard, \
this is the <emphasis>selection</emphasis>. It can be pasted using the middle mouse button.\
<nl/>\
<nl/>\
If the selection is explicitly copied using a <interface>Copy</interface> or <interface>Cut</interface> action, \
it is saved to the <emphasis>clipboard</emphasis>. It can be pasted using a <interface>Paste</interface> action. \
<nl/>\
<nl/>\
When turned on this option keeps the selection and the clipboard the same, so that any selection is immediately available to paste by any means. \
If it is turned off, the selection may still be saved in the clipboard history (subject to the options below), but it can only be pasted using the middle mouse button."),
                           hint);
    });

    layout->addRow(QString(), new QLabel(this));

    // Retain clipboard history
    item = KlipperSettings::self()->keepClipboardContentsItem();
    m_enableHistoryCb = new QCheckBox(item->label(), this);
    m_enableHistoryCb->setObjectName(QLatin1String("kcfg_KeepClipboardContents"));
    layout->addRow(i18n("Clipboard history:"), m_enableHistoryCb);

    // Clipboard history size
    item = KlipperSettings::self()->maxClipItemsItem();
    m_historySizeSb = new KPluralHandlingSpinBox(this);
    m_historySizeSb->setObjectName(QLatin1String("kcfg_MaxClipItems"));
    m_historySizeSb->setSuffix(ki18ncp("Number of entries", " entry", " entries"));
    layout->addRow(item->label(), m_historySizeSb);

    layout->addRow(QString(), new QLabel(this));

    // Radio button group: Storing text selections in history
    //
    // These two options correspond to the 'ignoreSelection' internal
    // Klipper setting.
    //
    // The 'Always' option is not available if selection/clipboard synchronisation
    // is turned off - in this case the selection is never automatically saved
    // in the clipboard history.

    QButtonGroup *buttonGroup = new QButtonGroup(this);

    // This widget is not managed by KConfigDialogManager, but
    // the other radio button is.  That is sufficient for the
    // manager to handle widget changes, the Apply button etc.
    m_alwaysTextRb = new QRadioButton(i18n("Always save in history"), this);
    m_alwaysTextRb->setChecked(true); // may be updated from settings later
    connect(m_alwaysTextRb, &QAbstractButton::toggled, this, &GeneralWidget::widgetChanged);
    buttonGroup->addButton(m_alwaysTextRb);
    layout->addRow(i18n("Text selection:"), m_alwaysTextRb);

    m_copiedTextRb = new QRadioButton(i18n("Only when explicitly copied"), this);
    m_copiedTextRb->setObjectName(QLatin1String("kcfg_IgnoreSelection"));
    buttonGroup->addButton(m_copiedTextRb);
    layout->addRow(QString(), m_copiedTextRb);

    layout->addRow(QString(), ConfigDialog::createHintLabel(i18n("Whether text selections are saved in the clipboard history."), this));

    // Radio button group: Storing non-text selections in history
    //
    // The truth table for the 4 possible combinations of internal Klipper
    // settings (of which two are equivalent, making 3 user-visible options)
    // controlling what is stored in the clipboard history is:
    //
    // selectionTextOnly  ignoreImages   Selected   Selected    Copied    Copied      Option
    //                                     text    image/other   text   image/other
    //
    //        false          false          yes       yes         yes       yes         1
    //        true           false          yes       no          yes       yes         2
    //        false          true           yes       no          yes       no          3
    //        true           true           yes       no          yes       no          3
    //
    // Option 1:  Always store images in history
    //        2:  Only when explicitly copied
    //        3:  Never store images in history
    //
    // The 'Always' option is not available if selection/clipboard synchronisation
    // is turned off.

    buttonGroup = new QButtonGroup(this);

    // This widget is not managed by KConfigDialogManager,
    // but the other two radio buttons are.
    m_alwaysImageRb = new QRadioButton(i18n("Always save in history"), this);
    m_alwaysImageRb->setChecked(true); // may be updated from settings later
    connect(m_alwaysImageRb, &QAbstractButton::toggled, this, &GeneralWidget::widgetChanged);
    buttonGroup->addButton(m_alwaysImageRb);
    layout->addRow(i18n("Non-text selection:"), m_alwaysImageRb);

    m_copiedImageRb = new QRadioButton(i18n("Only when explicitly copied"), this);
    m_copiedImageRb->setObjectName(QLatin1String("kcfg_SelectionTextOnly"));
    buttonGroup->addButton(m_copiedImageRb);
    layout->addRow(QString(), m_copiedImageRb);

    m_neverImageRb = new QRadioButton(i18n("Never save in history"), this);
    m_neverImageRb->setObjectName(QLatin1String("kcfg_IgnoreImages"));
    buttonGroup->addButton(m_neverImageRb);
    layout->addRow(QString(), m_neverImageRb);

    layout->addRow(QString(), ConfigDialog::createHintLabel(i18n("Whether non-text selections (such as images) are saved in the clipboard history."), this));

    m_settingsSaved = false;
}

void GeneralWidget::updateWidgets()
{
    // Initialise widgets which are not managed by KConfigDialogManager
    // from the application settings.

    // SelectionTextOnly takes precedence over IgnoreImages,
    // see Klipper::checkClipData().  Give that radio button
    // priority too.
    if (KlipperSettings::selectionTextOnly()) {
        KlipperSettings::setIgnoreImages(false);
    }
}

void GeneralWidget::slotWidgetModified()
{
    // A setting widget has been changed.  Update the state of
    // any other widgets that depend on it.

    if (m_syncClipboardsCb->isChecked()) {
        m_alwaysImageRb->setEnabled(true);
        m_alwaysTextRb->setEnabled(true);
        m_copiedTextRb->setEnabled(true);

        if (m_settingsSaved) {
            m_alwaysTextRb->setChecked(m_prevAlwaysText);
            m_alwaysImageRb->setChecked(m_prevAlwaysImage);
            m_settingsSaved = false;
        }
    } else {
        m_prevAlwaysText = m_alwaysTextRb->isChecked();
        m_prevAlwaysImage = m_alwaysImageRb->isChecked();
        m_settingsSaved = true;

        if (m_alwaysImageRb->isChecked()) {
            m_copiedImageRb->setChecked(true);
        }

        if (m_alwaysTextRb->isChecked()) {
            m_copiedTextRb->setChecked(true);
        }

        m_alwaysImageRb->setEnabled(false);
        m_alwaysTextRb->setEnabled(false);
        m_copiedTextRb->setEnabled(false);
    }
}

//////////////////////////
//  PopupWidget		//
//////////////////////////

PopupWidget::PopupWidget(QWidget *parent)
    : QWidget(parent)
{
    QFormLayout *layout = new QFormLayout(this);

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
                                                        "When text that matches an action pattern is selected or is chosen from \
the clipboard history, automatically show the popup menu with applicable actions. \
If the automatic menu is turned off here, or it is not shown for an excluded window, \
then it can be shown by using the <shortcut>%1</shortcut> key shortcut.",
                                                        ConfigDialog::manualShortcutString()),
                                                 this);
    layout->addRow(QString(), hint);

    // Exclusions
    QPushButton *exclusionsButton = new QPushButton(QIcon::fromTheme(QStringLiteral("configure")), i18n("Exclude Windows..."), this);
    connect(exclusionsButton, &QPushButton::clicked, this, &PopupWidget::onAdvanced);

    // Right align the push button, regardless of the QFormLayout style
    QHBoxLayout *hb = new QHBoxLayout;
    hb->setContentsMargins(0, 0, 0, 0);
    hb->addStretch(1);
    hb->addWidget(exclusionsButton);
    layout->addRow(QString(), hb);

    // Action popup time
    item = KlipperSettings::self()->timeoutForActionPopupsItem();
    m_actionTimeoutSb = new KPluralHandlingSpinBox(this);
    m_actionTimeoutSb->setObjectName(QLatin1String("kcfg_TimeoutForActionPopups"));
    m_actionTimeoutSb->setSuffix(ki18ncp("Unit of time", " second", " seconds"));
    m_actionTimeoutSb->setSpecialValueText(i18nc("No timeout", "None"));
    layout->addRow(item->label(), m_actionTimeoutSb);

    layout->addRow(QString(), new QLabel(this));

    // Remove whitespace
    item = KlipperSettings::self()->stripWhiteSpaceItem();
    m_stripWhitespaceCb = new QCheckBox(item->label(), this);
    m_stripWhitespaceCb->setObjectName(QLatin1String("kcfg_StripWhiteSpace"));
    layout->addRow(i18n("Options:"), m_stripWhitespaceCb);
    layout->addRow(QString(), ConfigDialog::createHintLabel(item, this));

    // MIME actions
    item = KlipperSettings::self()->enableMagicMimeActionsItem();
    m_mimeActionsCb = new QCheckBox(item->label(), this);
    m_mimeActionsCb->setObjectName(QLatin1String("kcfg_EnableMagicMimeActions"));
    layout->addRow(QString(), m_mimeActionsCb);
    layout->addRow(QString(), ConfigDialog::createHintLabel(item, this));

    layout->addRow(QString(), new QLabel(this));
}

void PopupWidget::setExcludedWMClasses(const QStringList &excludedWMClasses)
{
    m_exclWMClasses = excludedWMClasses;
}

QStringList PopupWidget::excludedWMClasses() const
{
    return m_exclWMClasses;
}

void PopupWidget::onAdvanced()
{
    QDialog dlg(this);
    dlg.setModal(true);
    dlg.setWindowTitle(i18n("Exclude Windows"));
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

//////////////////////////
//  ActionsWidget	//
//////////////////////////

ActionsWidget::ActionsWidget(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // General information label
    QLabel *hint = ConfigDialog::createHintLabel(xi18nc("@info",
                                                        "When a <interface>match pattern</interface> \
matches the clipboard contents, its <interface>commands</interface> \
appear in the Klipper popup menu and can be executed."),
                                                 this);
    layout->addWidget(hint, 0, 0, 1, -1);

    // Scrolling list
    m_actionsTree = new ActionsTreeWidget(this);
    m_actionsTree->setColumnCount(2);
    m_actionsTree->setHeaderLabels({i18nc("@title:column", "Match pattern and commands"), i18nc("@title:column", "Description")});

    layout->addWidget(m_actionsTree, 1, 0, 1, -1);
    layout->setRowStretch(1, 1);

    // Action buttons
    m_addActionButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add Action..."), this);
    connect(m_addActionButton, &QPushButton::clicked, this, &ActionsWidget::onAddAction);
    layout->addWidget(m_addActionButton, 2, 0);

    m_editActionButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-edit")), i18n("Edit Action..."), this);
    connect(m_editActionButton, &QPushButton::clicked, this, &ActionsWidget::onEditAction);
    layout->addWidget(m_editActionButton, 2, 1);
    layout->setColumnStretch(2, 1);

    m_deleteActionButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Delete Action"), this);
    connect(m_deleteActionButton, &QPushButton::clicked, this, &ActionsWidget::onDeleteAction);
    layout->addWidget(m_deleteActionButton, 2, 3);

    // Where to configure the action options
    if (KlipperSettings::actionsInfoMessageShown()) {
        KMessageWidget *msg = new KMessageWidget(xi18nc("@info",
                                                        "These actions appear in the popup menu \
which can be configured on the <interface>Action Menu</interface> page."),
                                                 this);
        msg->setMessageType(KMessageWidget::Information);
        msg->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
        msg->setWordWrap(true);
        msg->setCloseButtonVisible(true);

        connect(msg, &KMessageWidget::hideAnimationFinished, this, []() {
            KlipperSettings::setActionsInfoMessageShown(false);
        });
        layout->addWidget(msg, 3, 0, 1, -1);
    }

    // Add some vertical space between our buttons and the dialogue buttons
    layout->setRowMinimumHeight(4, 16);

    const KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
    QByteArray hdrState = grp.readEntry("ColumnState", QByteArray());
    if (!hdrState.isEmpty()) {
        qCDebug(KLIPPER_LOG) << "Restoring column state";
        m_actionsTree->header()->restoreState(QByteArray::fromBase64(hdrState));
    } else {
        m_actionsTree->header()->resizeSection(0, 250);
    }

    connect(m_actionsTree, &QTreeWidget::itemSelectionChanged, this, &ActionsWidget::onSelectionChanged);
    connect(m_actionsTree, &QTreeWidget::itemDoubleClicked, this, &ActionsWidget::onEditAction);

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

        QTreeWidgetItem *item = new QTreeWidgetItem;
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

    for (const ClipCommand &command : action->commands()) {
        QStringList cmdProps;
        cmdProps << command.command << command.description;
        QTreeWidgetItem *child = new QTreeWidgetItem(item, cmdProps);
        child->setIcon(0, QIcon::fromTheme(command.icon.isEmpty() ? QStringLiteral("system-run") : command.icon));
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
    KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
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
    ClipAction *newAct = new ClipAction;
    dlg.setAction(newAct);

    if (dlg.exec() == QDialog::Accepted) {
        m_actionList.append(newAct);

        QTreeWidgetItem *item = new QTreeWidgetItem;
        updateActionItem(item, newAct);
        m_actionsTree->addTopLevelItem(item);
        emit widgetChanged();
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
        emit widgetChanged();
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
        emit widgetChanged();
    }
}

bool ActionsWidget::hasChanged() const
{
    return (m_actionsTree->actionsChanged() != -1);
}

//////////////////////////
//  ConfigDialog	//
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

    m_shortcutsWidget = new KShortcutsEditor(collection, this, KShortcutsEditor::GlobalAction);
    addPage(m_shortcutsWidget, i18nc("Shortcuts Config", "Shortcuts"), QStringLiteral("preferences-desktop-keyboard"), i18n("Shortcuts Configuration"));

    connect(m_generalPage, &GeneralWidget::widgetChanged, this, &ConfigDialog::settingsChangedSlot);
    connect(m_actionsPage, &ActionsWidget::widgetChanged, this, &ConfigDialog::settingsChangedSlot);
    connect(this, &KConfigDialog::widgetModified, m_generalPage, &GeneralWidget::slotWidgetModified);

    // from KWindowConfig::restoreWindowSize() API documentation
    (void)winId();
    const KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
    KWindowConfig::restoreWindowSize(windowHandle(), grp);
    resize(windowHandle()->size());

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
    m_klipper->urlGrabber()->setExcludedWMClasses(m_popupPage->excludedWMClasses());
    m_klipper->saveSettings();

    KlipperSettings::self()->save();

    KConfigGroup grp = KSharedConfig::openConfig()->group("ConfigDialog");
    KWindowConfig::saveWindowSize(windowHandle(), grp);
}

void ConfigDialog::updateWidgets()
{
    // The dialogue is being shown.  Initialise widgets which are not
    // managed by KConfigDialogManager from the application settings.

    if (m_klipper && m_klipper->urlGrabber()) {
        m_actionsPage->setActionList(m_klipper->urlGrabber()->actionList());
        m_popupPage->setExcludedWMClasses(m_klipper->urlGrabber()->excludedWMClasses());
    } else {
        qCDebug(KLIPPER_LOG) << "Klipper or grabber object is null";
        return;
    }

    m_generalPage->updateWidgets();
}

void ConfigDialog::updateWidgetsDefault()
{
    // The user clicked "Defaults".  Restore the default values for
    // widgets which are not managed by KConfigDialogManager.  The
    // settings of "Actions Configuration" and "Excluded Windows"
    // are not reset to the default.

    m_shortcutsWidget->allDefault();
}

bool ConfigDialog::hasChanged()
{
    return (m_actionsPage->hasChanged() || m_shortcutsWidget->isModified());
}

//////////////////////////
//  AdvancedWidget	//
//////////////////////////

AdvancedWidget::AdvancedWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *hint = ConfigDialog::createHintLabel(xi18nc("@info",
                                                        "The action popup will not be shown automatically for these windows, \
even if it is enabled. This is because, for example, a web browser may highlight a URL \
in the address bar while typing, so the menu would show for every keystroke.\
<nl/>\
<nl/>\
If the action menu appears unexpectedly when using a particular application, then add it to this list. \
<link>How to find the name to enter</link>."),
                                                 this);

    mainLayout->addWidget(hint);
    connect(hint, &QLabel::linkActivated, this, [this, hint]() {
        QToolTip::showText(QCursor::pos(),
                           xi18nc("@info:tooltip",
                                  "The name that needs to be entered here is the WM_CLASS name of the window to be excluded. \
To find the WM_CLASS name for a window, in another terminal window enter the command:\
<nl/>\
<nl/>\
&nbsp;&nbsp;<icode>xprop | grep WM_CLASS</icode>\
<nl/>\
<nl/>\
and click on the window that you want to exclude. \
The first name that it displays after the equal sign is the one that you need to enter."),
                           hint);
    });

    mainLayout->addWidget(hint);
    mainLayout->addWidget(new QLabel(this));

    m_editListBox = new KEditListWidget(this);
    m_editListBox->setButtons(KEditListWidget::Add | KEditListWidget::Remove);
    m_editListBox->setCheckAtEntering(true);
    mainLayout->addWidget(m_editListBox);

    m_editListBox->setFocus();
}

void AdvancedWidget::setWMClasses(const QStringList &items)
{
    m_editListBox->setItems(items);
}

QStringList AdvancedWidget::wmClasses() const
{
    return m_editListBox->items();
}
