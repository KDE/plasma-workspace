/*
   This file is part of the KDE project
   Copyright (C) 2009 by Dmitry Suzdalev <dimsuz@gmail.com>

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

#include "editactiondialog.h"

#include <QIcon>
#include <QItemDelegate>
#include <QComboBox>
#include "klipper_debug.h"
#include <QDialogButtonBox>

#include <kwindowconfig.h>
#include <KWindowConfig>

#include "urlgrabber.h"
#include "ui_editactiondialog.h"

namespace {
    static QString output2text(ClipCommand::Output output) {
        switch(output) {
            case ClipCommand::IGNORE:
                return QString(i18n("Ignore"));
            case ClipCommand::REPLACE:
                return QString(i18n("Replace Clipboard"));
            case ClipCommand::ADD:
                return QString(i18n("Add to Clipboard"));
        }
        return QString();
    }

}

/**
 * Show dropdown of editing Output part of commands
 */
class ActionOutputDelegate : public QItemDelegate {
    public:
        ActionOutputDelegate(QObject* parent = nullptr) : QItemDelegate(parent){
        }

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const override {
            QComboBox* editor = new QComboBox(parent);
            editor->setInsertPolicy(QComboBox::NoInsert);
            editor->addItem(output2text(ClipCommand::IGNORE), QVariant::fromValue<ClipCommand::Output>(ClipCommand::IGNORE));
            editor->addItem(output2text(ClipCommand::REPLACE), QVariant::fromValue<ClipCommand::Output>(ClipCommand::REPLACE));
            editor->addItem(output2text(ClipCommand::ADD), QVariant::fromValue<ClipCommand::Output>(ClipCommand::ADD));
            return editor;

        }

        void setEditorData(QWidget* editor, const QModelIndex& index) const override {
            QComboBox* ed = static_cast<QComboBox*>(editor);
            QVariant data(index.model()->data(index, Qt::EditRole));
            ed->setCurrentIndex(static_cast<int>(data.value<ClipCommand::Output>()));
        }

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
            QComboBox* ed = static_cast<QComboBox*>(editor);
            model->setData(index, ed->itemData(ed->currentIndex()));
        }

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const override {
            editor->setGeometry(option.rect);
        }
};

class ActionDetailModel : public QAbstractTableModel {
    public:
        ActionDetailModel(ClipAction* action, QObject* parent = nullptr);
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        const QList<ClipCommand>& commands() const { return m_commands; }
        void addCommand(const ClipCommand& command);
        void removeCommand(const QModelIndex& index);

    private:
        enum column_t {
            COMMAND_COL = 0,
            OUTPUT_COL = 1,
            DESCRIPTION_COL = 2
        };
        QList<ClipCommand> m_commands;
        QVariant displayData(ClipCommand* command, column_t column) const;
        QVariant editData(ClipCommand* command, column_t column) const;
        QVariant decorationData(ClipCommand* command, column_t column) const;
        void setIconForCommand(ClipCommand& cmd);
};

ActionDetailModel::ActionDetailModel(ClipAction* action, QObject* parent):
    QAbstractTableModel(parent),
    m_commands(action->commands())
{

}

Qt::ItemFlags ActionDetailModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


void ActionDetailModel::setIconForCommand(ClipCommand& cmd)
{
    // let's try to update icon of the item according to command
    QString command = cmd.command;
    if ( command.contains( QLatin1Char(' ') ) ) {
        // get first word
        command = command.section( QLatin1Char(' '), 0, 0 );
    }

    if (QIcon::hasThemeIcon(command)) {
        cmd.icon = command;
    } else {
        cmd.icon.clear();
    }

}

bool ActionDetailModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole) {
        ClipCommand cmd = m_commands.at(index.row());
        switch (static_cast<column_t>(index.column())) {
            case COMMAND_COL:
                cmd.command = value.toString();
                setIconForCommand(cmd);
                break;
            case OUTPUT_COL:
                cmd.output = value.value<ClipCommand::Output>();
                break;
            case DESCRIPTION_COL:
                cmd.description = value.toString();
                break;
        }
        m_commands.replace(index.row(), cmd);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

int ActionDetailModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 3;
}

int ActionDetailModel::rowCount(const QModelIndex&) const
{
    return m_commands.count();
}

QVariant ActionDetailModel::displayData(ClipCommand* command, ActionDetailModel::column_t column) const
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

QVariant ActionDetailModel::decorationData(ClipCommand* command, ActionDetailModel::column_t column) const
{
    switch (column) {
        case COMMAND_COL:
            return command->icon.isEmpty() ? QIcon::fromTheme( QStringLiteral("system-run") ) : QIcon::fromTheme( command->icon );
        case OUTPUT_COL:
        case DESCRIPTION_COL:
            break;
    }
    return QVariant();

}

QVariant ActionDetailModel::editData(ClipCommand* command, ActionDetailModel::column_t column) const
{
    switch (column) {
        case COMMAND_COL:
            return command->command;
        case OUTPUT_COL:
            return QVariant::fromValue<ClipCommand::Output>(command->output);
        case DESCRIPTION_COL:
            return command->description;
    }
    return QVariant();

}

QVariant ActionDetailModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(static_cast<column_t>(section)) {
        case COMMAND_COL:
            return i18n("Command");
        case OUTPUT_COL:
            return i18n("Output Handling");
        case DESCRIPTION_COL:
            return i18n("Description");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}


QVariant ActionDetailModel::data(const QModelIndex& index, int role) const
{
    const int column = index.column();
    const int row = index.row();
    ClipCommand cmd = m_commands.at(row);
    switch (role) {
        case Qt::DisplayRole:
            return displayData(&cmd, static_cast<column_t>(column));
        case Qt::DecorationRole:
            return decorationData(&cmd, static_cast<column_t>(column));
        case Qt::EditRole:
            return editData(&cmd, static_cast<column_t>(column));
    }
    return QVariant();
}

void ActionDetailModel::addCommand(const ClipCommand& command) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_commands << command;
    endInsertRows();
}

void ActionDetailModel::removeCommand(const QModelIndex& index) {
    int row = index.row();
    beginRemoveRows(QModelIndex(), row, row);
    m_commands.removeAt(row);
    endRemoveRows();

}

EditActionDialog::EditActionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Action Properties"));
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, this, &EditActionDialog::slotAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QWidget* dlgWidget = new QWidget(this);
    m_ui = new Ui::EditActionDialog;
    m_ui->setupUi(dlgWidget);

    m_ui->leRegExp->setClearButtonEnabled(true);
    m_ui->leDescription->setClearButtonEnabled(true);

    m_ui->pbAddCommand->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_ui->pbRemoveCommand->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));

    // For some reason, the default row height is 30 pixel. Set it to the minimum sectionSize instead,
    // which is the font height+struts.
    m_ui->twCommandList->verticalHeader()->setDefaultSectionSize(m_ui->twCommandList->verticalHeader()->minimumSectionSize());
    m_ui->twCommandList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(dlgWidget);
    layout->addWidget(buttons);

    connect(m_ui->pbAddCommand, &QPushButton::clicked, this, &EditActionDialog::onAddCommand);
    connect(m_ui->pbRemoveCommand, &QPushButton::clicked, this, &EditActionDialog::onRemoveCommand);

    const KConfigGroup grp = KSharedConfig::openConfig()->group("EditActionDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), grp);
    QByteArray hdrState = grp.readEntry("ColumnState", QByteArray());
    if (!hdrState.isEmpty()) {
        qCDebug(KLIPPER_LOG) << "Restoring column state";
        m_ui->twCommandList->horizontalHeader()->restoreState(QByteArray::fromBase64(hdrState));
    }
							// do this after restoreState()
    m_ui->twCommandList->horizontalHeader()->setHighlightSections(false);
}

EditActionDialog::~EditActionDialog()
{
    delete m_ui;
}

void EditActionDialog::setAction(ClipAction* act, int commandIdxToSelect)
{
    m_action = act;
    m_model = new ActionDetailModel(act, this);
    m_ui->twCommandList->setModel(m_model);
    m_ui->twCommandList->setItemDelegateForColumn(1, new ActionOutputDelegate);
    connect(m_ui->twCommandList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EditActionDialog::onSelectionChanged);

    updateWidgets( commandIdxToSelect );
}

void EditActionDialog::updateWidgets(int commandIdxToSelect)
{
    if (!m_action) {
        qCDebug(KLIPPER_LOG) << "no action to edit was set";
        return;
    }

    m_ui->leRegExp->setText(m_action->actionRegexPattern());
    m_ui->automatic->setChecked(m_action->automatic());
    m_ui->leDescription->setText(m_action->description());

    if (commandIdxToSelect != -1) {
        m_ui->twCommandList->setCurrentIndex( m_model->index( commandIdxToSelect ,0 ) );
    }

    // update Remove button
    onSelectionChanged();
}

void EditActionDialog::saveAction()
{
    if (!m_action) {
        qCDebug(KLIPPER_LOG) << "no action to edit was set";
        return;
    }

    m_action->setActionRegexPattern( m_ui->leRegExp->text() );
    m_action->setDescription( m_ui->leDescription->text() );
    m_action->setAutomatic( m_ui->automatic->isChecked() );

    m_action->clearCommands();

    foreach ( const ClipCommand& cmd, m_model->commands() ){
        m_action->addCommand( cmd );
    }
}

void EditActionDialog::slotAccepted()
{
    saveAction();

    qCDebug(KLIPPER_LOG) << "Saving dialogue state";
    KConfigGroup grp = KSharedConfig::openConfig()->group("EditActionDialog");
    KWindowConfig::saveWindowSize(windowHandle(), grp);
    grp.writeEntry("ColumnState",
                    m_ui->twCommandList->horizontalHeader()->saveState().toBase64());
    accept();
}

void EditActionDialog::onAddCommand()
{
    m_model->addCommand(ClipCommand(i18n( "new command" ),
                                    i18n( "Command Description" ),
                                    true,
                                    QLatin1String("") ));
    m_ui->twCommandList->edit( m_model->index( m_model->rowCount()-1, 0 ));
}

void EditActionDialog::onRemoveCommand()
{
    m_model->removeCommand(m_ui->twCommandList->selectionModel()->currentIndex());
}

void EditActionDialog::onSelectionChanged()
{
    m_ui->pbRemoveCommand->setEnabled( m_ui->twCommandList->selectionModel() && m_ui->twCommandList->selectionModel()->hasSelection() );
}


