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
#include <qradiobutton.h>
#include <qtooltip.h>
#include <qwindow.h>

#include <KConfigSkeleton>
#include <KEditListWidget>
#include <KShortcutsEditor>
#include <kconfigskeleton.h>
#include <kglobalaccel.h>
#include <kpluralhandlingspinbox.h>
#include <kwindowconfig.h>
#include <kmessagewidget.h>

#include "klipper_debug.h"

#include "editactiondialog.h"
#include "klipper.h"
#include "klippersettings.h"

static QLabel *createHintLabel(const QString &text, QWidget *parent)
{
    QLabel *hintLabel = new QLabel(text, parent);
    hintLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    hintLabel->setWordWrap(true);
    hintLabel->setAlignment(Qt::AlignLeft|Qt::AlignTop);

    // The minimum width needs to be set so that QLabel will wrap the text
    // to fill that width.  Otherwise, it will try to adjust the text wrapping
    // width so that the text is laid out in a pleasing looking rectangle,
    // which unfortunately leaves a lot of white space below the actual text.
    // See the "tryWidth" block in QLabelPrivate::sizeForWidth().
    hintLabel->setMinimumWidth(400);

    return (hintLabel);
}

static QLabel *createHintLabel(const KConfigSkeletonItem *item, QWidget *parent)
{
    return (createHintLabel(item->whatsThis(), parent));
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
    m_syncClipboardsCb->setChecked(KlipperSettings::syncClipboards());
    layout->addRow(i18n("Selection and Clipboard:"), m_syncClipboardsCb);

    QLabel *hint = createHintLabel(item, this);
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
    m_enableHistoryCb->setChecked(KlipperSettings::keepClipboardContents());
    m_enableHistoryCb->setToolTip(item->toolTip());
    layout->addRow(i18n("Clipboard history:"), m_enableHistoryCb);

    // Clipboard history size
    item = KlipperSettings::self()->maxClipItemsItem();
    m_historySizeSb = new KPluralHandlingSpinBox(this);
    m_historySizeSb->setRange(item->minValue().toInt(), item->maxValue().toInt());
    m_historySizeSb->setValue(KlipperSettings::maxClipItems());
    m_historySizeSb->setSuffix(ki18ncp("Number of entries", " entry", " entries"));
    m_historySizeSb->setToolTip(item->toolTip());
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

    QButtonGroup *bg = new QButtonGroup(this);

    m_alwaysTextRb = new QRadioButton(i18n("Always save in history"), this);
    m_alwaysTextRb->setChecked(!KlipperSettings::ignoreSelection());
    bg->addButton(m_alwaysTextRb);
    layout->addRow(i18n("Text selection:"), m_alwaysTextRb);

    m_copiedTextRb = new QRadioButton(i18n("Only when explicitly copied"), this);
    m_copiedTextRb->setChecked(KlipperSettings::ignoreSelection());
    bg->addButton(m_copiedTextRb);
    layout->addRow(QString(), m_copiedTextRb);

    layout->addRow(QString(), createHintLabel(i18n("Whether text selections are saved in the clipboard history."), this));

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

    bg = new QButtonGroup(this);

    m_alwaysImageRb = new QRadioButton(i18n("Always save in history"), this);
    m_alwaysImageRb->setChecked(!KlipperSettings::ignoreImages() && !KlipperSettings::selectionTextOnly());
    bg->addButton(m_alwaysImageRb);
    layout->addRow(i18n("Non-text selection:"), m_alwaysImageRb);

    m_copiedImageRb = new QRadioButton(i18n("Only when explicitly copied"), this);
    m_copiedImageRb->setChecked(!KlipperSettings::ignoreImages() && KlipperSettings::selectionTextOnly());
    bg->addButton(m_copiedImageRb);
    layout->addRow(QString(), m_copiedImageRb);

    m_neverImageRb = new QRadioButton(i18n("Never save in history"), this);
    m_neverImageRb->setChecked(KlipperSettings::ignoreImages());
    bg->addButton(m_neverImageRb);
    layout->addRow(QString(), m_neverImageRb);

    layout->addRow(QString(), createHintLabel(i18n("Whether non-text selections (such as images) are saved in the clipboard history."), this));

    layout->addRow(QString(), new QLabel(this));

    m_settingsSaved = false;
    connect(m_syncClipboardsCb, &QAbstractButton::clicked, this, &GeneralWidget::updateWidgets);

    layout->addItem(new QSpacerItem(QSizePolicy::Fixed, QSizePolicy::Expanding));
}

void GeneralWidget::updateWidgets()
{
    if (m_syncClipboardsCb->isChecked()) {
        m_alwaysImageRb->setEnabled(true);
        m_alwaysTextRb->setEnabled(true);

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
    }
}

void GeneralWidget::save()
{
    KlipperSettings::setKeepClipboardContents(m_enableHistoryCb->isChecked());
    KlipperSettings::setSyncClipboards(m_syncClipboardsCb->isChecked());

    KlipperSettings::setIgnoreSelection(m_copiedTextRb->isChecked());

    KlipperSettings::setIgnoreImages(m_neverImageRb->isChecked());
    KlipperSettings::setSelectionTextOnly(m_copiedImageRb->isChecked());

    KlipperSettings::setMaxClipItems(m_historySizeSb->value());
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
    m_enablePopupCb->setChecked(KlipperSettings::uRLGrabberEnabled());
    m_enablePopupCb->setToolTip(item->toolTip());
    layout->addRow(i18n("Show action popup menu:"), m_enablePopupCb);

    // Replay from history popup
    item = KlipperSettings::self()->replayActionInHistoryItem();
    m_historyPopupCb = new QCheckBox(item->label(), this);
    m_historyPopupCb->setChecked(KlipperSettings::replayActionInHistory());
    m_historyPopupCb->setToolTip(item->toolTip());
    layout->addRow(QString(), m_historyPopupCb);

    const QList<QKeySequence> keys = KGlobalAccel::self()->globalShortcut(QCoreApplication::applicationName(), QStringLiteral("repeat_action"));
    layout->addRow(QString(),
                   createHintLabel(xi18nc("@info",
                                          "When text that matches an action pattern is selected or is chosen from \
the clipboard history, automatically show the popup menu with applicable actions. \
If the automatic menu is turned off here, or it is not shown for an excluded window, \
then it can be shown by using the <shortcut>%1</shortcut> key shortcut.",
                                          keys.value(0).toString(QKeySequence::NativeText)),
                                   this));
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
    m_actionTimeoutSb->setRange(item->minValue().toInt(), item->maxValue().toInt());
    m_actionTimeoutSb->setValue(KlipperSettings::timeoutForActionPopups());
    m_actionTimeoutSb->setSuffix(ki18ncp("Unit of time", " second", " seconds"));
    m_actionTimeoutSb->setSpecialValueText(i18nc("No timeout", "None"));
    m_actionTimeoutSb->setToolTip(item->toolTip());
    layout->addRow(item->label(), m_actionTimeoutSb);

    layout->addRow(QString(), new QLabel(this));

    // Remove whitespace
    item = KlipperSettings::self()->stripWhiteSpaceItem();
    m_stripWhitespaceCb = new QCheckBox(item->label(), this);
    m_stripWhitespaceCb->setChecked(KlipperSettings::stripWhiteSpace());
    layout->addRow(i18n("Options:"), m_stripWhitespaceCb);
    layout->addRow(QString(), createHintLabel(item, this));

    // MIME actions
    item = KlipperSettings::self()->enableMagicMimeActionsItem();
    m_mimeActionsCb = new QCheckBox(item->label(), this);
    m_mimeActionsCb->setChecked(KlipperSettings::enableMagicMimeActions());
    layout->addRow(QString(), m_mimeActionsCb);
    layout->addRow(QString(), createHintLabel(item, this));

    layout->addRow(QString(), new QLabel(this));

    // Where to configure the actions
    if (!KlipperSettings::popupInfoMessageHidden())
    {
        KMessageWidget *msg = new KMessageWidget(xi18nc("@info", "The actions shown in the popup menu \
can be configured on the <interface>Actions Configuration</interface> page."), this);
        msg->setMessageType(KMessageWidget::Information);
        msg->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
        msg->setWordWrap(true);
        msg->setCloseButtonVisible(true);

        connect(msg, &KMessageWidget::hideAnimationFinished, this, []() {
            KlipperSettings::setPopupInfoMessageHidden(true);
        });
        layout->addRow(msg);
    }
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

void PopupWidget::save()
{
    KlipperSettings::setURLGrabberEnabled(m_enablePopupCb->isChecked());
    KlipperSettings::setReplayActionInHistory(m_historyPopupCb->isChecked());

    KlipperSettings::setStripWhiteSpace(m_stripWhitespaceCb->isChecked());
    KlipperSettings::setEnableMagicMimeActions(m_mimeActionsCb->isChecked());

    KlipperSettings::setTimeoutForActionPopups(m_actionTimeoutSb->value());
}

//////////////////////////
//  ActionsWidget	//
//////////////////////////

ActionsWidget::ActionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_editActDlg(nullptr)
{
    m_ui.setupUi(this);

    m_ui.pbAddAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_ui.pbDelAction->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    m_ui.pbEditAction->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));

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

    connect(m_generalPage, &GeneralWidget::settingChanged, this, &ConfigDialog::settingsChangedSlot);
    connect(m_popupPage, &PopupWidget::settingChanged, this, &ConfigDialog::settingsChangedSlot);

    // from KWindowConfig::restoreWindowSize() API documentation
    (void) winId();
    const KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
    KWindowConfig::restoreWindowSize(windowHandle(), grp);
    resize(windowHandle()->size());
}

void ConfigDialog::updateSettings()
{
    // user clicked Ok or Apply

    if (!m_klipper) {
        qCDebug(KLIPPER_LOG) << "Klipper object is null";
        return;
    }

    m_shortcutsWidget->save();
    m_generalPage->save();
    m_popupPage->save();

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
    // settings were updated, update widgets

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
    // default widget values requested

    m_shortcutsWidget->allDefault();
}

//////////////////////////
//  AdvancedWidget	//
//////////////////////////

AdvancedWidget::AdvancedWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *hint = createHintLabel(xi18nc("@info",
                                          "The action popup menu will not be shown automatically for these windows, \
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
