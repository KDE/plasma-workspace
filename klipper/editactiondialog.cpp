/*
    SPDX-FileCopyrightText: 2009 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "editactiondialog.h"

#include <QDialogButtonBox>
#include <qcheckbox.h>
#include <qcoreapplication.h>
#include <qformlayout.h>
#include <qgridlayout.h>
#include <qheaderview.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtableview.h>
#include <qwindow.h>

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kwindowconfig.h>

#include "klipper_debug.h"

#include "configdialog.h"
#include "editcommanddialog.h"

static QString output2text(ClipCommand::Output output)
{
    switch (output) {
    case ClipCommand::IGNORE:
        return QString(i18n("Ignore"));
    case ClipCommand::REPLACE:
        return QString(i18n("Replace Clipboard"));
    case ClipCommand::ADD:
        return QString(i18n("Add to Clipboard"));
    }
    return QString();
}

//////////////////////////
//  ActionDetailModel	//
//////////////////////////

class ActionDetailModel : public QAbstractTableModel
{
public:
    explicit ActionDetailModel(ClipAction *action, QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    const QList<ClipCommand> &commands() const
    {
        return m_commands;
    }
    void addCommand(const ClipCommand &command);
    void removeCommand(const QModelIndex &idx);
    void replaceCommand(const ClipCommand &command, const QModelIndex &idx);

private:
    enum column_t { COMMAND_COL = 0, OUTPUT_COL = 1, DESCRIPTION_COL = 2 };
    QList<ClipCommand> m_commands;
    QVariant displayData(ClipCommand *command, column_t column) const;
    QVariant decorationData(ClipCommand *command, column_t column) const;
};

ActionDetailModel::ActionDetailModel(ClipAction *action, QObject *parent)
    : QAbstractTableModel(parent)
    , m_commands(action->commands())
{
}

Qt::ItemFlags ActionDetailModel::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int ActionDetailModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

int ActionDetailModel::rowCount(const QModelIndex &) const
{
    return m_commands.count();
}

QVariant ActionDetailModel::displayData(ClipCommand *command, ActionDetailModel::column_t column) const
{
    switch (column) {
    case COMMAND_COL:
        return command->command;
    case OUTPUT_COL:
        return output2text(command->output);
    case DESCRIPTION_COL:
        return command->description;
    }
    return QVariant();
}

QVariant ActionDetailModel::decorationData(ClipCommand *command, ActionDetailModel::column_t column) const
{
    switch (column) {
    case COMMAND_COL:
        return command->icon.isEmpty() ? QIcon::fromTheme(QStringLiteral("system-run")) : QIcon::fromTheme(command->icon);
    case OUTPUT_COL:
    case DESCRIPTION_COL:
        break;
    }
    return QVariant();
}

QVariant ActionDetailModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<column_t>(section)) {
        case COMMAND_COL:
            return i18n("Command");
        case OUTPUT_COL:
            return i18n("Output");
        case DESCRIPTION_COL:
            return i18n("Description");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant ActionDetailModel::data(const QModelIndex &index, int role) const
{
    const int column = index.column();
    const int row = index.row();
    ClipCommand cmd = m_commands.at(row);
    switch (role) {
    case Qt::DisplayRole:
        return displayData(&cmd, static_cast<column_t>(column));
    case Qt::DecorationRole:
        return decorationData(&cmd, static_cast<column_t>(column));
    }
    return QVariant();
}

void ActionDetailModel::addCommand(const ClipCommand &command)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_commands << command;
    endInsertRows();
}

void ActionDetailModel::replaceCommand(const ClipCommand &command, const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    const int row = idx.row();
    m_commands[row] = command;
    emit dataChanged(index(row, static_cast<int>(COMMAND_COL)), index(row, static_cast<int>(DESCRIPTION_COL)));
}

void ActionDetailModel::removeCommand(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    const int row = idx.row();
    beginRemoveRows(QModelIndex(), row, row);
    m_commands.removeAt(row);
    endRemoveRows();
}

//////////////////////////
//  EditActionDialog	//
//////////////////////////

EditActionDialog::EditActionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Action Properties"));
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, this, &EditActionDialog::slotAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Upper widget: pattern, description and options
    QWidget *optionsWidget = new QWidget(this);
    QFormLayout *optionsLayout = new QFormLayout(optionsWidget);

    // General information label
    QLabel *hint = ConfigDialog::createHintLabel(xi18nc("@info",
                                                        "An action takes effect when its \
<interface>match pattern</interface> matches the clipboard contents. \
When this happens, the action's <interface>commands</interface> appear \
in the Klipper popup menu; if one of them is chosen, \
the command is executed."),
                                                 this);
    optionsLayout->addRow(hint);
    optionsLayout->addRow(QString(), new QLabel(optionsWidget));

    // Pattern (regular expression)
    m_regExpEdit = new QLineEdit(optionsWidget);
    m_regExpEdit->setClearButtonEnabled(true);
    m_regExpEdit->setPlaceholderText(i18n("Enter a pattern to match against the clipboard"));

    optionsLayout->addRow(i18n("Match pattern:"), m_regExpEdit);

    hint = ConfigDialog::createHintLabel(xi18nc("@info",
                                                "The match pattern is a regular expression. \
For more information see the \
<link url=\"https://en.wikipedia.org/wiki/Regular_expression\">Wikipedia entry</link> \
for this topic."),
                                         this);
    hint->setOpenExternalLinks(true);
    optionsLayout->addRow(QString(), hint);

    // Description
    m_descriptionEdit = new QLineEdit(optionsWidget);
    m_descriptionEdit->setClearButtonEnabled(true);
    m_descriptionEdit->setPlaceholderText(i18n("Enter a description for the action"));
    optionsLayout->addRow(i18n("Description:"), m_descriptionEdit);

    // Include in automatic popup
    m_automaticCheck = new QCheckBox(i18n("Include in automatic popup"), optionsWidget);
    optionsLayout->addRow(QString(), m_automaticCheck);

    hint = ConfigDialog::createHintLabel(xi18nc("@info",
                                                "The commands \
for this match will be included in the automatic action popup, if it is enabled in \
the <interface>Action Menu</interface> page. If this option is turned off, the commands for \
this match will not be included in the automatic popup but they will be included if the \
popup is activated manually with the <shortcut>%1</shortcut> key shortcut.",
                                                ConfigDialog::manualShortcutString()),
                                         this);
    optionsLayout->addRow(QString(), hint);

    optionsLayout->addRow(QString(), new QLabel(optionsWidget));

    // Lower widget: command list and action buttons
    QWidget *listWidget = new QWidget(this);
    QGridLayout *listLayout = new QGridLayout(listWidget);
    listLayout->setContentsMargins(0, 0, 0, 0);

    // Command list
    m_commandList = new QTableView(listWidget);
    m_commandList->setAlternatingRowColors(true);
    m_commandList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_commandList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commandList->setShowGrid(false);
    m_commandList->setWordWrap(false);
    m_commandList->horizontalHeader()->setStretchLastSection(true);
    m_commandList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_commandList->verticalHeader()->setVisible(false);
    // For some reason, the default row height is 30 pixels.
    // Set it to the minimumSectionSize instead,
    // which is the font height+struts.
    m_commandList->verticalHeader()->setDefaultSectionSize(m_commandList->verticalHeader()->minimumSectionSize());

    listLayout->addWidget(m_commandList, 0, 0, 1, -1);
    listLayout->setRowStretch(0, 1);

    // "Add" button
    m_addCommandPb = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add Command..."), listWidget);
    connect(m_addCommandPb, &QPushButton::clicked, this, &EditActionDialog::onAddCommand);
    listLayout->addWidget(m_addCommandPb, 1, 0);

    // "Edit" button
    m_editCommandPb = new QPushButton(QIcon::fromTheme(QStringLiteral("document-edit")), i18n("Edit Command..."), this);
    connect(m_editCommandPb, &QPushButton::clicked, this, &EditActionDialog::onEditCommand);
    listLayout->addWidget(m_editCommandPb, 1, 1);
    listLayout->setColumnStretch(2, 1);

    // "Delete" button
    m_removeCommandPb = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Delete Command"), this);
    connect(m_removeCommandPb, &QPushButton::clicked, this, &EditActionDialog::onRemoveCommand);
    listLayout->addWidget(m_removeCommandPb, 1, 3);

    // Add some vertical space between our buttons and the dialogue buttons
    listLayout->setRowMinimumHeight(2, 16);

    // Main dialogue layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(optionsWidget);
    mainLayout->addWidget(listWidget);
    mainLayout->setStretch(1, 1);
    mainLayout->addWidget(buttons);

    (void)winId();
    windowHandle()->resize(540, 560); // default, if there is no saved size
    const KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
    KWindowConfig::restoreWindowSize(windowHandle(), grp);
    resize(windowHandle()->size());

    QByteArray hdrState = grp.readEntry("ColumnState", QByteArray());
    if (!hdrState.isEmpty()) {
        qCDebug(KLIPPER_LOG) << "Restoring column state";
        m_commandList->horizontalHeader()->restoreState(QByteArray::fromBase64(hdrState));
    }
    // do this after restoreState()
    m_commandList->horizontalHeader()->setHighlightSections(false);
}

void EditActionDialog::setAction(ClipAction *act, int commandIdxToSelect)
{
    m_action = act;
    m_model = new ActionDetailModel(act, this);
    m_commandList->setModel(m_model);
    connect(m_commandList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EditActionDialog::onSelectionChanged);
    connect(m_commandList, &QAbstractItemView::doubleClicked, this, &EditActionDialog::onEditCommand);
    updateWidgets(commandIdxToSelect);
}

void EditActionDialog::updateWidgets(int commandIdxToSelect)
{
    if (!m_action) {
        qCDebug(KLIPPER_LOG) << "no action to edit was set";
        return;
    }

    m_regExpEdit->setText(m_action->actionRegexPattern());
    m_descriptionEdit->setText(m_action->description());
    m_automaticCheck->setChecked(m_action->automatic());

    if (commandIdxToSelect != -1) {
        m_commandList->setCurrentIndex(m_model->index(commandIdxToSelect, 0));
    }

    onSelectionChanged(); // update Remove/Edit buttons
}

void EditActionDialog::saveAction()
{
    if (!m_action) {
        qCDebug(KLIPPER_LOG) << "no action to edit was set";
        return;
    }

    m_action->setActionRegexPattern(m_regExpEdit->text());
    m_action->setDescription(m_descriptionEdit->text());
    m_action->setAutomatic(m_automaticCheck->isChecked());

    m_action->clearCommands();

    foreach (const ClipCommand &cmd, m_model->commands()) {
        m_action->addCommand(cmd);
    }
}

void EditActionDialog::slotAccepted()
{
    saveAction();

    qCDebug(KLIPPER_LOG) << "Saving dialogue state";
    KConfigGroup grp = KSharedConfig::openConfig()->group(metaObject()->className());
    KWindowConfig::saveWindowSize(windowHandle(), grp);
    grp.writeEntry("ColumnState", m_commandList->horizontalHeader()->saveState().toBase64());
    accept();
}

void EditActionDialog::onAddCommand()
{
    ClipCommand command(QString(), QString(), true, QLatin1String(""));
    EditCommandDialog dlg(command, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_model->addCommand(dlg.command());
}

void EditActionDialog::onEditCommand()
{
    QPersistentModelIndex commandIndex(m_commandList->selectionModel()->currentIndex());
    if (!commandIndex.isValid())
        return;

    EditCommandDialog dlg(m_model->commands().at(commandIndex.row()), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_model->replaceCommand(dlg.command(), commandIndex);
}

void EditActionDialog::onRemoveCommand()
{
    QPersistentModelIndex commandIndex(m_commandList->selectionModel()->currentIndex());
    if (!commandIndex.isValid())
        return;

    if (KMessageBox::warningContinueCancel(
            this,
            xi18nc("@info", "Delete the selected command <resource>%1</resource>?", m_model->commands().at(commandIndex.row()).description),
            i18n("Confirm Delete Command"),
            KStandardGuiItem::del(),
            KStandardGuiItem::cancel(),
            QStringLiteral("deleteCommand"),
            KMessageBox::Dangerous)
        == KMessageBox::Continue) {
        m_model->removeCommand(commandIndex);
    }
}

void EditActionDialog::onSelectionChanged()
{
    const bool itemIsSelected = (m_commandList->selectionModel() && m_commandList->selectionModel()->hasSelection());
    m_removeCommandPb->setEnabled(itemIsSelected);
    m_editCommandPb->setEnabled(itemIsSelected);
}
