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

#include <QItemDelegate>
#include <QComboBox>

#include <KDebug>
#include <KIconLoader>

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
        ActionOutputDelegate(QObject* parent = 0) : QItemDelegate(parent){
        }

        virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const {
            QComboBox* editor = new QComboBox(parent);
            editor->setInsertPolicy(QComboBox::NoInsert);
            editor->addItem(output2text(ClipCommand::IGNORE), QVariant::fromValue<ClipCommand::Output>(ClipCommand::IGNORE));
            editor->addItem(output2text(ClipCommand::REPLACE), QVariant::fromValue<ClipCommand::Output>(ClipCommand::REPLACE));
            editor->addItem(output2text(ClipCommand::ADD), QVariant::fromValue<ClipCommand::Output>(ClipCommand::ADD));
            return editor;

        }

        virtual void setEditorData(QWidget* editor, const QModelIndex& index) const {
            QComboBox* ed = static_cast<QComboBox*>(editor);
            QVariant data(index.model()->data(index, Qt::EditRole));
            ed->setCurrentIndex(static_cast<int>(data.value<ClipCommand::Output>()));
        }

        virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
            QComboBox* ed = static_cast<QComboBox*>(editor);
            model->setData(index, ed->itemData(ed->currentIndex()));
        }

        virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const {
            editor->setGeometry(option.rect);
        }
};

class ActionDetailModel : public QAbstractTableModel {
    public:
        ActionDetailModel(ClipAction* action, QObject* parent = 0);
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
        virtual Qt::ItemFlags flags(const QModelIndex& index) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& parent) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
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
        QVariant displayData(ClipCommand* command, column_t colunm) const;
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
    if ( command.contains( ' ' ) ) {
        // get first word
        command = command.section( ' ', 0, 0 );
    }

    QPixmap iconPix = KIconLoader::global()->loadIcon(
                                        command, KIconLoader::Small, 0,
                                        KIconLoader::DefaultState,
                                        QStringList(), 0, true /* canReturnNull */ );

    if ( !iconPix.isNull() ) {
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
                cmd.command = value.value<QString>();
                setIconForCommand(cmd);
                break;
            case OUTPUT_COL:
                cmd.output = value.value<ClipCommand::Output>();
                break;
            case DESCRIPTION_COL:
                cmd.description = value.value<QString>();
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
            return command->icon.isEmpty() ? QIcon::fromTheme( "system-run" ) : QIcon::fromTheme( command->icon );
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
    : KDialog(parent)
{
    setCaption(i18n("Action Properties"));
    setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget* dlgWidget = new QWidget(this);
    m_ui = new Ui::EditActionDialog;
    m_ui->setupUi(dlgWidget);

    m_ui->leRegExp->setClearButtonShown(true);
    m_ui->leDescription->setClearButtonShown(true);

    m_ui->pbAddCommand->setIcon(QIcon::fromTheme("list-add"));
    m_ui->pbRemoveCommand->setIcon(QIcon::fromTheme("list-remove"));

    // For some reason, the default row height is 30 pixel. Set it to the minimum sectionSize instead,
    // which is the font height+struts.
    m_ui->twCommandList->verticalHeader()->setDefaultSectionSize(m_ui->twCommandList->verticalHeader()->minimumSectionSize());
    m_ui->twCommandList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    setMainWidget(dlgWidget);

    connect(m_ui->pbAddCommand, SIGNAL(clicked()), SLOT(onAddCommand()) );
    connect(m_ui->pbRemoveCommand, SIGNAL(clicked()), SLOT(onRemoveCommand()) );

    const KConfigGroup grp = KSharedConfig::openConfig()->group("EditActionDialog");
    restoreDialogSize(grp);
    QByteArray hdrState = grp.readEntry("ColumnState", QByteArray());
    if (!hdrState.isEmpty()) {
        kDebug() << "Restoring column state";
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
    connect(m_ui->twCommandList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(onSelectionChanged()));

    updateWidgets( commandIdxToSelect );
}

void EditActionDialog::updateWidgets(int commandIdxToSelect)
{
    if (!m_action) {
        kDebug() << "no action to edit was set";
        return;
    }

    m_ui->leRegExp->setText(m_action->regExp());
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
        kDebug() << "no action to edit was set";
        return;
    }

    m_action->setRegExp( m_ui->leRegExp->text() );
    m_action->setDescription( m_ui->leDescription->text() );
    m_action->setAutomatic( m_ui->automatic->isChecked() );

    m_action->clearCommands();

    foreach ( const ClipCommand& cmd, m_model->commands() ){
        m_action->addCommand( cmd );
    }
}

void EditActionDialog::slotButtonClicked( int button )
{
    if ( button == KDialog::Ok ) {
        saveAction();

        kDebug() << "Saving dialogue state";
        KConfigGroup grp = KSharedConfig::openConfig()->group("EditActionDialog");
        saveDialogSize(grp);
        grp.writeEntry("ColumnState",
                       m_ui->twCommandList->horizontalHeader()->saveState().toBase64());
    }

    KDialog::slotButtonClicked( button );
}

void EditActionDialog::onAddCommand()
{
    m_model->addCommand(ClipCommand(i18n( "new command" ),
                                    i18n( "Command Description" ),
                                    true,
                                    "" ));
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

#include "editactiondialog.moc"
