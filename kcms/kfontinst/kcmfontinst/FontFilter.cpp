/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-FileCopyrightText: 2019 Guo Yunhe <i@guoyunhe.me>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontFilter.h"
#include "FontFilterProxyStyle.h"
#include "FontList.h"
#include <KIconLoader>
#include <KSelectAction>
#include <KToggleAction>
#include <QApplication>
#include <QLabel>
#include <QMimeDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QSet>
#include <QString>
#include <QStyleOption>

namespace KFI
{
static void deselectCurrent(QActionGroup *act)
{
    QAction *prev(act->checkedAction());
    if (prev) {
        prev->setChecked(false);
    }
}

static void deselectCurrent(KSelectAction *act)
{
    deselectCurrent(act->selectableActionGroup());
}

// FIXME: Go back to using StyleSheets instead of a proxy style
// once Qt has been fixed not to mess with widget font when
// using StyleSheets
class CFontFilterStyle : public CFontFilterProxyStyle
{
public:
    CFontFilterStyle(CFontFilter *parent, int ol)
        : CFontFilterProxyStyle(parent)
        , overlap(ol)
    {
    }

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override;

    int overlap;
};

QRect CFontFilterStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    if (SE_LineEditContents == element) {
        QRect rect(style()->subElementRect(SE_LineEditContents, option, widget));

        return rect.adjusted(overlap, 0, -overlap, 0);
    }

    return CFontFilterProxyStyle::subElementRect(element, option, widget);
}

struct SortAction {
    SortAction(QAction *a)
        : action(a)
    {
    }
    bool operator<(const SortAction &o) const
    {
        return action->text().localeAwareCompare(o.action->text()) < 0;
    }
    QAction *action;
};

static void sortActions(KSelectAction *group)
{
    if (group->actions().count() > 1) {
        QList<QAction *> actions = group->actions();
        QList<QAction *>::ConstIterator it(actions.constBegin()), end(actions.constEnd());
        QList<SortAction> sorted;

        for (; it != end; ++it) {
            sorted.append(SortAction(*it));
            group->removeAction(*it);
        }

        std::sort(sorted.begin(), sorted.end());
        QList<SortAction>::ConstIterator s(sorted.constBegin()), sEnd(sorted.constEnd());

        for (; s != sEnd; ++s) {
            group->addAction((*s).action);
        }
    }
}

CFontFilter::CFontFilter(QWidget *parent)
    : QWidget(parent)
{
    m_icons[CRIT_FAMILY] = QIcon::fromTheme("draw-text");
    m_texts[CRIT_FAMILY] = i18n("Family");
    m_icons[CRIT_STYLE] = QIcon::fromTheme("format-text-bold");
    m_texts[CRIT_STYLE] = i18n("Style");
    m_icons[CRIT_FOUNDRY] = QIcon::fromTheme("user-identity");
    m_texts[CRIT_FOUNDRY] = i18n("Foundry");
    m_icons[CRIT_FONTCONFIG] = QIcon::fromTheme("system-search");
    m_texts[CRIT_FONTCONFIG] = i18n("FontConfig Match");
    m_icons[CRIT_FILETYPE] = QIcon::fromTheme("preferences-desktop-font-installer");
    m_texts[CRIT_FILETYPE] = i18n("File Type");
    m_icons[CRIT_FILENAME] = QIcon::fromTheme("application-x-font-type1");
    m_texts[CRIT_FILENAME] = i18n("File Name");
    m_icons[CRIT_LOCATION] = QIcon::fromTheme("folder");
    m_texts[CRIT_LOCATION] = i18n("File Location");
    m_icons[CRIT_WS] = QIcon::fromTheme("character-set");
    m_texts[CRIT_WS] = i18n("Writing System");

    m_layout = new QHBoxLayout(this);
    setLayout(m_layout);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setClearButtonEnabled(true);
    m_layout->addWidget(m_lineEdit);

    m_menuButton = new QPushButton(this);
    m_menuButton->setIcon(QIcon::fromTheme("view-filter"));
    m_menuButton->setText(i18n("Set Criteria"));
    m_layout->addWidget(m_menuButton);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &CFontFilter::textChanged);

    m_menu = new QMenu(this);
    m_menuButton->setMenu(m_menu);

    m_actionGroup = new QActionGroup(this);
    addAction(CRIT_FAMILY, true);
    addAction(CRIT_STYLE, false);

    KSelectAction *foundryMenu = new KSelectAction(m_icons[CRIT_FOUNDRY], m_texts[CRIT_FOUNDRY], this);
    m_actions[CRIT_FOUNDRY] = foundryMenu;
    m_menu->addAction(m_actions[CRIT_FOUNDRY]);
    foundryMenu->setData((int)CRIT_FOUNDRY);
    foundryMenu->setVisible(false);
    connect(foundryMenu, SIGNAL(triggered(QString)), SLOT(foundryChanged(QString)));

    addAction(CRIT_FONTCONFIG, false);

    KSelectAction *ftMenu = new KSelectAction(m_icons[CRIT_FILETYPE], m_texts[CRIT_FILETYPE], this);
    m_actions[CRIT_FILETYPE] = ftMenu;
    m_menu->addAction(m_actions[CRIT_FILETYPE]);
    ftMenu->setData((int)CRIT_FILETYPE);

    QStringList::ConstIterator it(CFontList::fontMimeTypes.constBegin()), end(CFontList::fontMimeTypes.constEnd());
    QMimeDatabase db;
    for (; it != end; ++it) {
        if ((*it) != "application/vnd.kde.fontspackage") {
            QMimeType mime = db.mimeTypeForName(*it);

            KToggleAction *act = new KToggleAction(QIcon::fromTheme(mime.iconName()), mime.comment(), this);

            ftMenu->addAction(act);
            act->setChecked(false);

            QStringList mimes;
            foreach (QString pattern, mime.globPatterns())
                mimes.append(pattern.remove(QStringLiteral("*.")));
            act->setData(mimes);
        }
    }

    sortActions(ftMenu);
    connect(ftMenu, SIGNAL(triggered(QString)), SLOT(ftChanged(QString)));
    m_currentFileTypes.clear();

    addAction(CRIT_FILENAME, false);
    addAction(CRIT_LOCATION, false);

    KSelectAction *wsMenu = new KSelectAction(m_icons[CRIT_WS], m_texts[CRIT_WS], this);
    m_actions[CRIT_WS] = wsMenu;
    m_menu->addAction(m_actions[CRIT_WS]);
    wsMenu->setData((int)CRIT_WS);

    m_currentWs = QFontDatabase::Any;
    for (int i = QFontDatabase::Latin; i < QFontDatabase::WritingSystemsCount; ++i) {
        KToggleAction *wsAct =
            new KToggleAction(QFontDatabase::Other == i ? i18n("Symbol/Other") : QFontDatabase::writingSystemName((QFontDatabase::WritingSystem)i), this);

        wsMenu->addAction(wsAct);
        wsAct->setChecked(false);
        wsAct->setData(i);
    }
    sortActions(wsMenu);
    connect(wsMenu, SIGNAL(triggered(QString)), SLOT(wsChanged(QString)));

    setCriteria(CRIT_FAMILY);
    setStyle(new CFontFilterStyle(this, m_menuButton->width()));
}

void CFontFilter::setFoundries(const QSet<QString> &currentFoundries)
{
    QAction *act(((KSelectAction *)m_actions[CRIT_FOUNDRY])->currentAction());
    QString prev(act && act->isChecked() ? act->text() : QString());
    bool changed(false);
    QList<QAction *> prevFoundries(((KSelectAction *)m_actions[CRIT_FOUNDRY])->actions());
    QList<QAction *>::ConstIterator fIt(prevFoundries.constBegin()), fEnd(prevFoundries.constEnd());
    QSet<QString> foundries(currentFoundries);

    // Determine which of 'foundries' are new ones, and which old ones need to be removed...
    for (; fIt != fEnd; ++fIt) {
        if (foundries.contains((*fIt)->text())) {
            foundries.remove((*fIt)->text());
        } else {
            ((KSelectAction *)m_actions[CRIT_FOUNDRY])->removeAction(*fIt);
            (*fIt)->deleteLater();
            changed = true;
        }
    }

    if (!foundries.isEmpty()) {
        // Add foundries to menu - replacing '&' with '&&', as '&' is taken to be
        // a shortcut!
        QSet<QString>::ConstIterator it(foundries.begin()), end(foundries.end());

        for (; it != end; ++it) {
            QString foundry(*it);

            foundry.replace("&", "&&");
            ((KSelectAction *)m_actions[CRIT_FOUNDRY])->addAction(foundry);
        }
        changed = true;
    }

    if (changed) {
        sortActions((KSelectAction *)m_actions[CRIT_FOUNDRY]);
        if (!prev.isEmpty()) {
            act = ((KSelectAction *)m_actions[CRIT_FOUNDRY])->action(prev);
            if (act) {
                ((KSelectAction *)m_actions[CRIT_FOUNDRY])->setCurrentAction(act);
            } else {
                ((KSelectAction *)m_actions[CRIT_FOUNDRY])->setCurrentItem(0);
            }
        }

        m_actions[CRIT_FOUNDRY]->setVisible(((KSelectAction *)m_actions[CRIT_FOUNDRY])->actions().count());
    }
}

void CFontFilter::filterChanged()
{
    QAction *act(m_actionGroup->checkedAction());

    if (act) {
        ECriteria crit((ECriteria)act->data().toInt());

        if (m_currentCriteria != crit) {
            deselectCurrent((KSelectAction *)m_actions[CRIT_FOUNDRY]);
            deselectCurrent((KSelectAction *)m_actions[CRIT_FILETYPE]);
            deselectCurrent((KSelectAction *)m_actions[CRIT_WS]);
            m_lineEdit->setText(QString());
            m_currentWs = QFontDatabase::Any;
            m_currentFileTypes.clear();

            setCriteria(crit);
            m_lineEdit->setPlaceholderText(i18n("Filter by %1…", act->text()));
            m_lineEdit->setReadOnly(false);
        }
    }
}

void CFontFilter::ftChanged(const QString &ft)
{
    deselectCurrent((KSelectAction *)m_actions[CRIT_FOUNDRY]);
    deselectCurrent((KSelectAction *)m_actions[CRIT_WS]);
    deselectCurrent(m_actionGroup);

    QAction *act(((KSelectAction *)m_actions[CRIT_FILETYPE])->currentAction());

    if (act) {
        m_currentFileTypes = act->data().toStringList();
    }
    m_currentCriteria = CRIT_FILETYPE;
    m_lineEdit->setReadOnly(true);
    setCriteria(m_currentCriteria);
    m_lineEdit->setText(ft);
    m_lineEdit->setPlaceholderText(m_lineEdit->text());
}

void CFontFilter::wsChanged(const QString &writingSystemName)
{
    deselectCurrent((KSelectAction *)m_actions[CRIT_FOUNDRY]);
    deselectCurrent((KSelectAction *)m_actions[CRIT_FILETYPE]);
    deselectCurrent(m_actionGroup);

    QAction *act(((KSelectAction *)m_actions[CRIT_WS])->currentAction());

    if (act) {
        m_currentWs = (QFontDatabase::WritingSystem)act->data().toInt();
    }
    m_currentCriteria = CRIT_WS;
    m_lineEdit->setReadOnly(true);
    setCriteria(m_currentCriteria);
    m_lineEdit->setText(writingSystemName);
    m_lineEdit->setPlaceholderText(m_lineEdit->text());
}

void CFontFilter::foundryChanged(const QString &foundry)
{
    deselectCurrent((KSelectAction *)m_actions[CRIT_WS]);
    deselectCurrent((KSelectAction *)m_actions[CRIT_FILETYPE]);
    deselectCurrent(m_actionGroup);

    m_currentCriteria = CRIT_FOUNDRY;
    m_lineEdit->setReadOnly(true);
    m_lineEdit->setText(foundry);
    m_lineEdit->setPlaceholderText(m_lineEdit->text());
    setCriteria(m_currentCriteria);
}

void CFontFilter::textChanged(const QString &text)
{
    Q_EMIT queryChanged(text);
}

void CFontFilter::addAction(ECriteria crit, bool on)
{
    m_actions[crit] = new KToggleAction(m_icons[crit], m_texts[crit], this);
    m_menu->addAction(m_actions[crit]);
    m_actionGroup->addAction(m_actions[crit]);
    m_actions[crit]->setData((int)crit);
    m_actions[crit]->setChecked(on);
    if (on) {
        m_lineEdit->setPlaceholderText(i18n("Filter by %1…", m_texts[crit]));
    }
    connect(m_actions[crit], &QAction::toggled, this, &CFontFilter::filterChanged);
}

void CFontFilter::setCriteria(ECriteria crit)
{
    m_currentCriteria = crit;

    Q_EMIT criteriaChanged(crit, ((qulonglong)1) << (int)m_currentWs, m_currentFileTypes);
}

}
