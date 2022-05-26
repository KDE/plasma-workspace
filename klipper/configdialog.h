/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KConfigDialog>

#include "urlgrabber.h"

class KConfigSkeleton;
class KConfigSkeletonItem;
class KShortcutsEditor;
class Klipper;
class KEditListWidget;
class KActionCollection;
class KPluralHandlingSpinBox;
class EditActionDialog;
class QCheckBox;
class QRadioButton;
class QTreeWidgetItem;
class QLabel;
class ActionsTreeWidget;

class GeneralWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralWidget(QWidget *parent);
    ~GeneralWidget() override = default;

    void updateWidgets();

Q_SIGNALS:
    void widgetChanged();

public Q_SLOTS:
    void slotWidgetModified();

private:
    QCheckBox *m_enableHistoryCb;
    QCheckBox *m_syncClipboardsCb;

    QRadioButton *m_alwaysTextRb;
    QRadioButton *m_copiedTextRb;

    QRadioButton *m_alwaysImageRb;
    QRadioButton *m_copiedImageRb;
    QRadioButton *m_neverImageRb;

    KPluralHandlingSpinBox *m_historySizeSb;

    bool m_settingsSaved;
    bool m_prevAlwaysImage;
    bool m_prevAlwaysText;
};

class PopupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PopupWidget(QWidget *parent);
    ~PopupWidget() override = default;

    void setExcludedWMClasses(const QStringList &);
    QStringList excludedWMClasses() const;

private Q_SLOTS:
    void onAdvanced();

private:
    QCheckBox *m_enablePopupCb;
    QCheckBox *m_historyPopupCb;
    QCheckBox *m_stripWhitespaceCb;
    QCheckBox *m_mimeActionsCb;

    KPluralHandlingSpinBox *m_actionTimeoutSb;

    QStringList m_exclWMClasses;
};

class ActionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ActionsWidget(QWidget *parent);
    ~ActionsWidget() override = default;

    void setActionList(const ActionList &);
    ActionList actionList() const;

    void resetModifiedState();
    bool hasChanged() const;

Q_SIGNALS:
    void widgetChanged();

private Q_SLOTS:
    void onSelectionChanged();
    void onAddAction();
    void onEditAction();
    void onDeleteAction();

private:
    void updateActionItem(QTreeWidgetItem *item, const ClipAction *action);
    void updateActionListView();

private:
    ActionsTreeWidget *m_actionsTree;
    QPushButton *m_addActionButton;
    QPushButton *m_editActionButton;
    QPushButton *m_deleteActionButton;

    /**
     * List of actions this page works with
     */
    ActionList m_actionList;
};

// only for use inside PopupWidget
class AdvancedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedWidget(QWidget *parent = nullptr);
    ~AdvancedWidget() override = default;

    void setWMClasses(const QStringList &items);
    QStringList wmClasses() const;

private:
    KEditListWidget *m_editListBox;
};

class ConfigDialog : public KConfigDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget *parent, KConfigSkeleton *config, Klipper *klipper, KActionCollection *collection);
    ~ConfigDialog() override = default;

    static QLabel *createHintLabel(const QString &text, QWidget *parent);
    static QLabel *createHintLabel(const KConfigSkeletonItem *item, QWidget *parent);
    static QString manualShortcutString();

protected:
    // reimp
    bool hasChanged() override;

protected slots:
    // reimp
    void updateWidgets() override;
    // reimp
    void updateSettings() override;
    // reimp
    void updateWidgetsDefault() override;

private:
    GeneralWidget *m_generalPage;
    PopupWidget *m_popupPage;
    ActionsWidget *m_actionsPage;
    KShortcutsEditor *m_shortcutsWidget;

    Klipper *m_klipper;
};
