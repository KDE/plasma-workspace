/* ColorEdit widget for KDE Display color scheme setup module
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KColorScheme>
#include <KSharedConfig>

#include <QDialog>
#include <QFrame>
#include <QPalette>

#include "ui_scmeditordialog.h"

class SchemeEditorOptions;
class SchemeEditorColors;
class SchemeEditorEffects;

class SchemeEditorDialog : public QDialog, public Ui::ScmEditorDialog
{
    Q_OBJECT

public:
    SchemeEditorDialog(const QString &path, QWidget *parent = nullptr);
    SchemeEditorDialog(KSharedConfigPtr config, QWidget *parent = nullptr);

    bool showApplyOverwriteButton() const;
    void setShowApplyOverwriteButton(bool show);

Q_SIGNALS:
    void changed(bool);

private Q_SLOTS:

    void on_buttonBox_clicked(QAbstractButton *button);

    void updateTabs(bool byUser = false);

private:
    void init();
    /** save the current scheme */
    void saveScheme(bool overwrite);
    void setUnsavedChanges(bool changes);

    const QString m_filePath;
    QString m_schemeName;
    KSharedConfigPtr m_config;
    bool m_disableUpdates = false;
    bool m_unsavedChanges = false;

    SchemeEditorOptions *m_optionTab;
    SchemeEditorColors *m_colorTab;
    SchemeEditorEffects *m_disabledTab;
    SchemeEditorEffects *m_inactiveTab;

    bool m_showApplyOverwriteButton = false;
};
