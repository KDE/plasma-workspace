/*
  * This file is part of the KDE project
  * Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
*/

#include "progresslistdelegate.h"
#include "progresslistdelegate_p.h"
#include "progresslistmodel.h"

#include <QApplication>
#include <QPushButton>
#include <QPainter>
#include <QHash>
#include <QFontMetrics>
#include <QListView>
#include <QProgressBar>
#include <KLocalizedString>

#define MIN_CONTENT_PIXELS 50

QString ProgressListDelegate::Private::getIcon(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::Icon).toString();
}

QString ProgressListDelegate::Private::getSizeTotal(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::SizeTotal).toString();
}

QString ProgressListDelegate::Private::getSizeProcessed(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::SizeProcessed).toString();
}

qlonglong ProgressListDelegate::Private::getTimeTotal(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::TimeTotal).toLongLong();
}

qlonglong ProgressListDelegate::Private::getTimeProcessed(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::TimeElapsed).toLongLong();
}

QString ProgressListDelegate::Private::getSpeed(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::Speed).toString();
}

int ProgressListDelegate::Private::getPercent(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::Percent).toInt();
}

QString ProgressListDelegate::Private::getInfoMessage(const QModelIndex &index) const
{
    return index.model()->data(index, JobView::InfoMessage).toString();
}

int ProgressListDelegate::Private::getCurrentLeftMargin(int fontHeight) const
{
    return leftMargin + separatorPixels + fontHeight;
}

ProgressListDelegate::ProgressListDelegate(QObject *parent, QListView *listView)
        : KWidgetItemDelegate(listView, parent)
        , d(new Private(listView))
{
}

ProgressListDelegate::~ProgressListDelegate()
{
    delete d;
}

void ProgressListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);
    if (!index.isValid()) {
        return;
    }

    QFontMetrics fontMetrics = painter->fontMetrics();
    int textHeight = fontMetrics.height();

    int coordY = d->separatorPixels + option.rect.top();

    QIcon iconToShow = QIcon::fromTheme(d->getIcon(index));

    QColor unselectedTextColor = option.palette.text().color();
    QColor selectedTextColor = option.palette.highlightedText().color();
    QPen currentPen = painter->pen();
    QPen unselectedPen = QPen(currentPen);
    QPen selectedPen = QPen(currentPen);

    unselectedPen.setColor(unselectedTextColor);
    selectedPen.setColor(selectedTextColor);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(selectedPen);
    } else {
        painter->setPen(unselectedPen);
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRect canvas = option.rect;
    int iconWidth = canvas.height() / 2 - d->leftMargin - d->rightMargin;
    int iconHeight = iconWidth;
    d->iconWidth = iconWidth;

    painter->drawPixmap(option.rect.right() - iconWidth - d->rightMargin, coordY, iconToShow.pixmap(iconWidth, iconHeight));

    if (!d->getInfoMessage(index).isEmpty()) {
        QString textToShow = fontMetrics.elidedText(d->getInfoMessage(index), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);

        textHeight = fontMetrics.size(Qt::TextSingleLine, textToShow).height();

        painter->drawText(d->getCurrentLeftMargin(textHeight) + option.rect.left(), coordY, fontMetrics.width(textToShow), textHeight, Qt::AlignLeft, textToShow);

        coordY += textHeight;
    }

    if (!d->getSizeProcessed(index).isEmpty() || !d->getSizeTotal(index).isEmpty() || !d->getSpeed(index).isEmpty()) {
        QString textToShow;
        if (!d->getSizeTotal(index).isEmpty() && !d->getSpeed(index).isEmpty())
            textToShow = fontMetrics.elidedText(i18n("%1 of %2 processed at %3/s", d->getSizeProcessed(index), d->getSizeTotal(index), d->getSpeed(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);
        else if (!d->getSizeTotal(index).isEmpty() && d->getSpeed(index).isEmpty())
            textToShow = fontMetrics.elidedText(i18n("%1 of %2 processed", d->getSizeProcessed(index), d->getSizeTotal(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);
        else if (d->getSizeTotal(index).isEmpty() && !d->getSpeed(index).isEmpty())
            textToShow = fontMetrics.elidedText(i18n("%1 processed at %2/s", d->getSizeProcessed(index), d->getSpeed(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);
        else
            textToShow = fontMetrics.elidedText(i18n("%1 processed", d->getSizeProcessed(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);

        textHeight = fontMetrics.size(Qt::TextSingleLine, textToShow).height();

        painter->drawText(d->getCurrentLeftMargin(textHeight) + option.rect.left(), coordY, fontMetrics.width(textToShow), textHeight, Qt::AlignLeft, textToShow);

        coordY += textHeight;
    }

    painter->restore();
}

QSize ProgressListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QFontMetrics fontMetrics = option.fontMetrics;

    int itemHeight = d->separatorPixels;
    int itemWidth = d->leftMargin + d->rightMargin + d->iconWidth + d->separatorPixels * 2 +
                    fontMetrics.height();

    int textSize = fontMetrics.height();

    if (!d->getInfoMessage(index).isEmpty()) {
        textSize = fontMetrics.size(Qt::TextSingleLine, d->getInfoMessage(index)).height();
        itemHeight += textSize;
    }

    if (!d->getSizeProcessed(index).isEmpty() || !d->getSpeed(index).isEmpty() ||
            !d->getSizeTotal(index).isEmpty()) {
        textSize = fontMetrics.size(Qt::TextSingleLine, d->getSizeProcessed(index)).height();
        itemHeight += textSize;
    }

    if (d->getPercent(index) > 0) {
        itemHeight += d->progressBar->sizeHint().height();
    }

    if (d->editorHeight > 0)
        itemHeight += d->editorHeight;

    if (itemHeight + d->separatorPixels >= d->minimumItemHeight)
        itemHeight += d->separatorPixels;
    else
        itemHeight = d->minimumItemHeight;

    return QSize(itemWidth + MIN_CONTENT_PIXELS, itemHeight);
}

void ProgressListDelegate::setSeparatorPixels(int separatorPixels)
{
    d->separatorPixels = separatorPixels;
}

void ProgressListDelegate::setLeftMargin(int leftMargin)
{
    d->leftMargin = leftMargin;
}

void ProgressListDelegate::setRightMargin(int rightMargin)
{
    d->rightMargin = rightMargin;
}

void ProgressListDelegate::setMinimumItemHeight(int minimumItemHeight)
{
    d->minimumItemHeight = minimumItemHeight;
}

void ProgressListDelegate::setMinimumContentWidth(int minimumContentWidth)
{
    d->minimumContentWidth = minimumContentWidth;
}

void ProgressListDelegate::setEditorHeight(int editorHeight)
{
    d->editorHeight = editorHeight;
}

QList<QWidget*> ProgressListDelegate::createItemWidgets(const QModelIndex &index) const
{
    Q_UNUSED( index )
    QList<QWidget*> widgetList;

    QPushButton *pauseResumeButton = new QPushButton();
    pauseResumeButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));

    QPushButton *cancelButton = new QPushButton();
    cancelButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));

    QPushButton *clearButton = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-clear")), i18n("Clear"));
    QProgressBar *progressBar = new QProgressBar();

    connect(pauseResumeButton, &QAbstractButton::clicked, this, &ProgressListDelegate::slotPauseResumeClicked);
    connect(cancelButton, &QAbstractButton::clicked, this, &ProgressListDelegate::slotCancelClicked);
    connect(clearButton, &QAbstractButton::clicked, this, &ProgressListDelegate::slotClearClicked);

    setBlockedEventTypes(pauseResumeButton, QList<QEvent::Type>() << QEvent::MouseButtonPress
                         << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick);
    setBlockedEventTypes(cancelButton, QList<QEvent::Type>() << QEvent::MouseButtonPress
                         << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick);

    widgetList << pauseResumeButton << cancelButton << progressBar << clearButton;

    return widgetList;
}

void ProgressListDelegate::updateItemWidgets(const QList<QWidget*> widgets,
        const QStyleOptionViewItem &option,
        const QPersistentModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    QPushButton *pauseResumeButton = static_cast<QPushButton*>(widgets[0]);

    QPushButton *cancelButton = static_cast<QPushButton*>(widgets[1]);
    cancelButton->setToolTip(i18n("Cancel"));

    QProgressBar *progressBar = static_cast<QProgressBar*>(widgets[2]);
    QPushButton *clearButton = static_cast<QPushButton*>(widgets[3]);

    int percent = d->getPercent(index);

    cancelButton->setVisible(percent < 100);
    pauseResumeButton->setVisible(percent < 100);
    clearButton->setVisible(percent > 99);

    KJob::Capabilities capabilities = (KJob::Capabilities) index.model()->data(index, JobView::Capabilities).toInt();
    cancelButton->setEnabled(capabilities & KJob::Killable);
    pauseResumeButton->setEnabled(capabilities & KJob::Suspendable);


    JobView::JobState state = (JobView::JobState) index.model()->data(index, JobView::State).toInt();
    switch (state) {
    case JobView::Running:
        pauseResumeButton->setToolTip(i18n("Pause"));
        pauseResumeButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
        break;
    case JobView::Suspended:
        pauseResumeButton->setToolTip(i18n("Resume"));
        pauseResumeButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        break;
    default:
        Q_ASSERT(0);
        break;
    }

    QSize progressBarButtonSizeHint;



    if (percent < 100) {
        QSize cancelButtonSizeHint = cancelButton->sizeHint();

        cancelButton->move(option.rect.width() - d->separatorPixels - cancelButtonSizeHint.width(),
                           option.rect.height() - d->separatorPixels - cancelButtonSizeHint.height());

        QSize pauseResumeButtonSizeHint = pauseResumeButton->sizeHint();


        pauseResumeButton->move(option.rect.width() - d->separatorPixels * 2 - pauseResumeButtonSizeHint.width() - cancelButtonSizeHint.width(),
                                option.rect.height() - d->separatorPixels - pauseResumeButtonSizeHint.height());

        progressBarButtonSizeHint = pauseResumeButtonSizeHint;
    } else {
        progressBarButtonSizeHint = clearButton->sizeHint();
        clearButton->resize(progressBarButtonSizeHint);

        clearButton->move(option.rect.width() - d->separatorPixels - progressBarButtonSizeHint.width(),
                          option.rect.height() - d->separatorPixels - progressBarButtonSizeHint.height());
    }
    progressBar->setValue(percent);

    QFontMetrics fm(QApplication::font());
    QSize progressBarSizeHint = progressBar->sizeHint();

    progressBar->resize(QSize(option.rect.width() - d->getCurrentLeftMargin(fm.height()) - d->rightMargin, progressBarSizeHint.height()));

    progressBar->move(d->getCurrentLeftMargin(fm.height()),
                      option.rect.height() - d->separatorPixels * 2 - progressBarButtonSizeHint.height() - progressBarSizeHint.height());
}

void ProgressListDelegate::slotPauseResumeClicked()
{
    const QModelIndex index = focusedIndex();
    JobView *jobView = index.model()->data(index, JobView::JobViewRole).value<JobView*>();
    JobView::JobState state = (JobView::JobState) index.model()->data(index, JobView::State).toInt();
    if (jobView) {
        switch (state) {
        case JobView::Running:
            jobView->requestSuspend();
            break;
        case JobView::Suspended:
            jobView->requestResume();
            break;
        default:
            Q_ASSERT(0); // this point should have never been reached
            break;
        }
    }
}

void ProgressListDelegate::slotCancelClicked()
{
    const QModelIndex index = focusedIndex();
    JobView *jobView = index.model()->data(index, JobView::JobViewRole).value<JobView*>();
    if (jobView) {
        jobView->requestCancel();
    }
}

void ProgressListDelegate::slotClearClicked()
{
    const QModelIndex index = focusedIndex();
    JobView *jobView = index.model()->data(index, JobView::JobViewRole).value<JobView*>();
    if (jobView) {
        jobView->terminate(QString());
    }
}


