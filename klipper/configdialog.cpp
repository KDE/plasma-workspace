/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2008-2009 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolTip>
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
//  GeneralWidget	//
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

    // Radio button group: Storing text selections in history
    //
    // These two options correspond to the 'ignoreSelection' internal
    // Klipper setting.
    //
    // The 'Always' option is not available if selection/clipboard synchronisation
    // is turned off - in this case the selection is never automatically saved
    // in the clipboard history.

    auto *buttonGroup = new QButtonGroup(this);

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

    m_havePrevAlwaysImageTextConfig = false;
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

void GeneralWidget::initWidgetStates()
{
    Q_ASSERT(!m_havePrevAlwaysImageTextConfig);
    // During dialog setup, disable / change some widgets according to current settings to achieve the same
    // internal consistency as settings changes made by the user after opening the dialog.
    slotWidgetModified();
    m_havePrevAlwaysImageTextConfig = false;
}

void GeneralWidget::slotWidgetModified()
{
    // A setting widget has been changed.  Update the state of
    // any other widgets that depend on it.

    if (m_syncClipboardsCb->isChecked()) {
        m_alwaysImageRb->setEnabled(true);
        m_alwaysTextRb->setEnabled(true);
        m_copiedTextRb->setEnabled(true);

        if (m_havePrevAlwaysImageTextConfig) {
            m_alwaysTextRb->setChecked(m_prevAlwaysText);
            m_alwaysImageRb->setChecked(m_prevAlwaysImage);
            m_havePrevAlwaysImageTextConfig = false;
        }
    } else {
        m_prevAlwaysText = m_alwaysTextRb->isChecked();
        m_prevAlwaysImage = m_alwaysImageRb->isChecked();
        m_havePrevAlwaysImageTextConfig = true;

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
//  ConfigDialog	//
//////////////////////////

ConfigDialog::ConfigDialog(QWidget *parent, KConfigSkeleton *skeleton, Klipper *klipper, KActionCollection *collection)
    : KConfigDialog(parent, QStringLiteral("preferences"), skeleton)
    , m_generalPage(new GeneralWidget(this))
    , m_klipper(klipper)
{
    addPage(m_generalPage, i18nc("General Config", "General"), QStringLiteral("klipper"), i18n("General Configuration"));

    m_shortcutsWidget = new KShortcutsEditor(collection, this, KShortcutsEditor::GlobalAction);
    addPage(m_shortcutsWidget, i18nc("Shortcuts Config", "Shortcuts"), QStringLiteral("preferences-desktop-keyboard"), i18n("Shortcuts Configuration"));

    connect(m_generalPage, &GeneralWidget::widgetChanged, this, &ConfigDialog::settingsChangedSlot);

    connect(this, &KConfigDialog::widgetModified, m_generalPage, &GeneralWidget::slotWidgetModified);
    m_generalPage->initWidgetStates();

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

    m_klipper->setURLGrabberEnabled(KlipperSettings::uRLGrabberEnabled());
    m_klipper->saveSettings();

    KlipperSettings::self()->save();

    KConfigGroup grp = KSharedConfig::openStateConfig()->group(u"klipper"_s).group(u"ConfigDialog"_s);
    KWindowConfig::saveWindowSize(windowHandle(), grp);
}

void ConfigDialog::updateWidgets()
{
    // The dialogue is being shown.  Initialise widgets which are not
    // managed by KConfigDialogManager from the application settings.

    m_generalPage->updateWidgets();
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
    return m_shortcutsWidget->isModified();
}

#include "moc_configdialog.cpp"
