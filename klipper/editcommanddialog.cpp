/*
    SPDX-FileCopyrightText: 2022 Jonathan Marten <jjm@keelhaul.me.uk>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "editcommanddialog.h"

#include <qbuttongroup.h>
#include <qdialogbuttonbox.h>
#include <qformlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qwindow.h>

#include <kiconbutton.h>
#include <klocalizedstring.h>
#include <kstandardguiitem.h>
#include <kwindowconfig.h>

#include "klipper_debug.h"

#include "configdialog.h"

static void setIconForCommand(ClipCommand *cmd)
{
    // let's try to update icon of the item according to command
    QString command = cmd->command;
    if (command.contains(QLatin1Char(' '))) {
        // get first word
        command = command.section(QLatin1Char(' '), 0, 0);
    }

    if (QIcon::hasThemeIcon(command)) {
        cmd->icon = command;
    } else {
        cmd->icon.clear();
    }
}

EditCommandDialog::EditCommandDialog(const ClipCommand &command, QWidget *parent)
    : QDialog(parent)
    , m_command(command)
{
    setWindowTitle(i18n("Command Properties"));
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, this, &EditCommandDialog::slotAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QWidget *optionsWidget = new QWidget(this);
    QFormLayout *optionsLayout = new QFormLayout(optionsWidget);

    // Command
    m_commandEdit = new QLineEdit(optionsWidget);
    m_commandEdit->setClearButtonEnabled(true);
    m_commandEdit->setPlaceholderText(i18n("Enter the command and arguments"));
    connect(m_commandEdit, &QLineEdit::textEdited, this, &EditCommandDialog::slotUpdateButtons);

    optionsLayout->addRow(i18n("Command:"), m_commandEdit);

    // See ClipCommandProcess::ClipCommandProcess() for the
    // substitutions made.  "%0" is the complete text matched by the
    // regular expression, which may only match a part of the clipboard
    // contents, so it is mentioned here.  However, "%u"/"%U" and "%f"/"%F"
    // are exactly equivalent to "%s", the complete clipboard contents,
    // so there is no point mentioning them here.
    QLabel *hint = ConfigDialog::createHintLabel(xi18nc("@info",
                                                        "A <placeholder>&#37;s</placeholder> in the command will be replaced by the \
complete clipboard contents. <placeholder>&#37;0</placeholder> through \
<placeholder>&#37;9</placeholder> will be replaced by the corresponding \
captured texts from the match pattern."),
                                                 optionsWidget);
    optionsLayout->addRow(QString(), hint);

    // Description
    m_descriptionEdit = new QLineEdit(optionsWidget);
    m_descriptionEdit->setClearButtonEnabled(true);
    m_descriptionEdit->setPlaceholderText(i18n("Enter a description for the command"));
    connect(m_descriptionEdit, &QLineEdit::textEdited, this, &EditCommandDialog::slotUpdateButtons);
    optionsLayout->addRow(i18n("Description:"), m_descriptionEdit);
    optionsLayout->addRow(QString(), new QLabel(this));

    // Radio button group: Output handling
    QButtonGroup *buttonGroup = new QButtonGroup(this);

    m_ignoreRadio = new QRadioButton(i18n("Ignore"), this);
    buttonGroup->addButton(m_ignoreRadio);
    optionsLayout->addRow(i18n("Output from command:"), m_ignoreRadio);

    m_replaceRadio = new QRadioButton(i18n("Replace current clipboard"), this);
    buttonGroup->addButton(m_replaceRadio);
    optionsLayout->addRow(QString(), m_replaceRadio);

    m_appendRadio = new QRadioButton(i18n("Append to clipboard"), this);
    buttonGroup->addButton(m_appendRadio);
    optionsLayout->addRow(QString(), m_appendRadio);

    optionsLayout->addRow(QString(), ConfigDialog::createHintLabel(i18n("What happens to the standard output of the command executed."), this));

    optionsLayout->addRow(QString(), new QLabel(this));

    // Icon and reset button
    QHBoxLayout *hb = new QHBoxLayout;
    hb->setContentsMargins(0, 0, 0, 0);

    m_iconButton = new KIconButton(this);
    m_iconButton->setIconSize(KIconLoader::SizeSmall);
    hb->addWidget(m_iconButton);

    QPushButton *resetButton = new QPushButton(this);
    KStandardGuiItem::assign(resetButton, KStandardGuiItem::Reset);
    resetButton->setToolTip(i18n("Reset the icon to the default for the command"));
    connect(resetButton, &QAbstractButton::clicked, this, [this]() {
        setIconForCommand(&m_command);
        m_iconButton->setIcon(m_command.icon);
    });
    hb->addWidget(resetButton);
    optionsLayout->addRow(i18n("Icon:"), hb);

    // Main dialogue layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(optionsWidget);
    mainLayout->addStretch();
    mainLayout->addWidget(buttons);

    (void)winId();
    windowHandle()->resize(560, 440); // default, if there is no saved size
    const KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
    KWindowConfig::restoreWindowSize(windowHandle(), grp);
    resize(windowHandle()->size());

    updateWidgets();
}

void EditCommandDialog::slotUpdateButtons()
{
    m_okButton->setEnabled(!m_commandEdit->text().isEmpty() && !m_descriptionEdit->text().isEmpty());
}

void EditCommandDialog::updateWidgets()
{
    m_commandEdit->setText(m_command.command);
    m_descriptionEdit->setText(m_command.description);

    m_replaceRadio->setChecked(m_command.output == ClipCommand::REPLACE);
    m_appendRadio->setChecked(m_command.output == ClipCommand::ADD);
    m_ignoreRadio->setChecked(m_command.output == ClipCommand::IGNORE);

    m_iconButton->setIcon(m_command.icon);

    slotUpdateButtons();
}

void EditCommandDialog::saveCommand()
{
    m_command.command = m_commandEdit->text();
    m_command.description = m_descriptionEdit->text();

    if (m_replaceRadio->isChecked()) {
        m_command.output = ClipCommand::REPLACE;
    } else if (m_appendRadio->isChecked()) {
        m_command.output = ClipCommand::ADD;
    } else {
        m_command.output = ClipCommand::IGNORE;
    }

    const QString icon = m_iconButton->icon();
    if (!icon.isEmpty()) {
        m_command.icon = icon;
    } else {
        setIconForCommand(&m_command);
    }
}

void EditCommandDialog::slotAccepted()
{
    saveCommand();

    KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
    KWindowConfig::saveWindowSize(windowHandle(), grp);
    accept();
}
