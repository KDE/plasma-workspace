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
class KActionCollection;
class QCheckBox;
class QRadioButton;
class QSpinBox;
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
    void initWidgetStates();

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

    QSpinBox *m_historySizeSb;

    bool m_havePrevAlwaysImageTextConfig;
    bool m_prevAlwaysImage;
    bool m_prevAlwaysText;
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

protected Q_SLOTS:
    // reimp
    void updateWidgets() override;
    // reimp
    void updateSettings() override;
    // reimp
    void updateWidgetsDefault() override;

private:
    GeneralWidget *m_generalPage;
    KShortcutsEditor *m_shortcutsWidget;

    Klipper *m_klipper;
};
