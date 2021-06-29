/*
    kcmformats.h
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __kcmformats_h__
#define __kcmformats_h__

#include <KCModule>
#include <KConfigGroup>

#include <QHash>
#include <QIcon>

namespace Ui
{
class KCMFormatsWidget;
}
class QComboBox;

class KCMFormats : public KCModule
{
    Q_OBJECT

public:
    explicit KCMFormats(QWidget *parent = nullptr, const QVariantList &list = QVariantList());
    ~KCMFormats() override;

    void load() override;
    void save() override;
    void defaults() override;

private:
    void addLocaleToCombo(QComboBox *combo, const QLocale &locale);
    void initCombo(QComboBox *combo, const QList<QLocale> &allLocales);
    void connectCombo(QComboBox *combo);
    QList<QComboBox *> m_combos;

    QIcon loadFlagIcon(const QString &flagCode);
    QHash<QString, QIcon> m_cachedFlags;
    QIcon m_cachedUnknown;

    void readConfig();
    void writeConfig();

    void updateExample();
    void updateEnabled();

    Ui::KCMFormatsWidget *m_ui;
    KConfigGroup m_config;
};

#endif
