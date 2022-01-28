/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KConfigDialog>

#include "urlgrabber.h"

#include "ui_actionsconfig.h"

class KConfigSkeleton;
class KShortcutsEditor;
class Klipper;
class KEditListWidget;
class KActionCollection;
class KPluralHandlingSpinBox;
class EditActionDialog;
class QCheckBox;
class QRadioButton;

class GeneralWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GeneralWidget(QWidget *parent);

    void updateWidgets();
    void save();

signals:
    void settingChanged();

private:
    QCheckBox *m_enableHistoryCb;
    QCheckBox *m_syncClipboardsCb;

    QRadioButton *m_alwaysTextRb;
    QRadioButton *m_copiedTextRb;

    QRadioButton *m_alwaysImageRb;
    QRadioButton *m_copiedImageRb;
    QRadioButton *m_neverImageRb;

    KPluralHandlingSpinBox *m_actionTimeoutSb;
    KPluralHandlingSpinBox *m_historySizeSb;

    bool m_settingsSaved;
    bool m_prevAlwaysImage;
    bool m_prevAlwaysText;
};

class ActionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ActionsWidget(QWidget *parent);

    void setActionList(const ActionList &);
    void setExcludedWMClasses(const QStringList &);

    ActionList actionList() const;
    QStringList excludedWMClasses() const;

    void resetModifiedState();

private Q_SLOTS:
    void onSelectionChanged();
    void onAddAction();
    void onEditAction();
    void onDeleteAction();
    void onAdvanced();

private:
    void updateActionItem(QTreeWidgetItem *item, ClipAction *action);
    void updateActionListView();

    Ui::ActionsWidget m_ui;
    EditActionDialog *m_editActDlg;

    /**
     * List of actions this page works with
     */
    ActionList m_actionList;

    QStringList m_exclWMClasses;
};

// only for use inside ActionWidget
class AdvancedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedWidget(QWidget *parent = nullptr);
    ~AdvancedWidget() override;

    void setWMClasses(const QStringList &items);
    QStringList wmClasses() const;

private:
    KEditListWidget *editListBox;
};

class ConfigDialog : public KConfigDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget *parent, KConfigSkeleton *config, const Klipper *klipper, KActionCollection *collection);
    ~ConfigDialog() override;

protected slots:
    // reimp
    void updateWidgets() override;
    // reimp
    void updateSettings() override;
    // reimp
    void updateWidgetsDefault() override;

private:
    GeneralWidget *m_generalPage;
    ActionsWidget *m_actionsPage;
    KShortcutsEditor *m_shortcutsWidget;

    const Klipper *m_klipper;
};
