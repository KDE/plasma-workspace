/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Printer.h"
#include "ActionLabel.h"
#include "FcEngine.h"
#include "config-fontinst.h"
#include <KAboutData>
#include <QApplication>
#include <QCloseEvent>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QProgressBar>
#include <QTextStream>
#include <QWidget>

#include "config-workspace.h"

#ifdef HAVE_LOCALE_H
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <locale.h>
#endif
#include "CreateParent.h"

// Enable the following to allow printing of non-installed fonts. Does not seem to work :-(
//#define KFI_PRINT_APP_FONTS

using namespace KFI;

static const int constMarginLineBefore = 1;
static const int constMarginLineAfter = 2;
static const int constMarginFont = 4;

inline bool sufficientSpace(int y, int pageHeight, const QFontMetrics &fm)
{
    return (y + constMarginFont + fm.height()) < pageHeight;
}

static bool sufficientSpace(int y, QPainter *painter, QFont font, const int *sizes, int pageHeight, int size)
{
    int titleFontHeight = painter->fontMetrics().height(), required = titleFontHeight + constMarginLineBefore + constMarginLineAfter;

    for (unsigned int s = 0; sizes[s]; ++s) {
        font.setPointSize(sizes[s]);
        required += QFontMetrics(font, painter->device()).height();
        if (sizes[s + 1]) {
            required += constMarginFont;
        }
    }

    if (0 == size) {
        font.setPointSize(CFcEngine::constDefaultAlphaSize);
        int fontHeight = QFontMetrics(font, painter->device()).height();

        required += (3 * (constMarginFont + fontHeight)) + constMarginLineBefore + constMarginLineAfter;
    }
    return (y + required) < pageHeight;
}

static QString usableStr(QFont &font, const QString &str)
{
    Q_UNUSED(font)
    return str;
}

static bool hasStr(QFont &font, const QString &str)
{
    Q_UNUSED(font)
    Q_UNUSED(str)
    return true;
}

static QString previewString(QFont &font, const QString &text, bool onlyDrawChars)
{
    Q_UNUSED(font)
    Q_UNUSED(onlyDrawChars)
    return text;
}

CPrintThread::CPrintThread(QPrinter *printer, const QList<Misc::TFont> &items, int size, QObject *parent)
    : QThread(parent)
    , m_printer(printer)
    , m_items(items)
    , m_size(size)
    , m_cancelled(false)
{
}

CPrintThread::~CPrintThread()
{
}

void CPrintThread::cancel()
{
    m_cancelled = true;
}

void CPrintThread::run()
{
    QPainter painter;
    QFont sans("sans", 12, QFont::Bold);
    bool changedFontEmbeddingSetting(false);
    QString str(CFcEngine(false).getPreviewString());

    if (!m_printer->fontEmbeddingEnabled()) {
        m_printer->setFontEmbeddingEnabled(true);
        changedFontEmbeddingSetting = true;
    }

    m_printer->setResolution(72);
    painter.begin(m_printer);

    int pageWidth = painter.device()->width(), pageHeight = painter.device()->height(), y = 0, oneSize[2] = {m_size, 0};
    const int *sizes = oneSize;
    bool firstFont(true);

    if (0 == m_size) {
        sizes = CFcEngine::constScalableSizes;
    }

    painter.setClipping(true);
    painter.setClipRect(0, 0, pageWidth, pageHeight);

    QList<Misc::TFont>::ConstIterator it(m_items.constBegin()), end(m_items.constEnd());

    for (int i = 0; it != end && !m_cancelled; ++it, ++i) {
        QString name(FC::createName((*it).family, (*it).styleInfo));
        Q_EMIT progress(i, name);

        unsigned int s = 0;
        QFont font;

#ifdef KFI_PRINT_APP_FONTS
        QString family;

        if (-1 != appFont[(*it).family]) {
            family = QFontDatabase::applicationFontFamilies(appFont[(*it).family]).first();
            font = QFont(family);
        }
#else
        font = CFcEngine::getQFont((*it).family, (*it).styleInfo, CFcEngine::constDefaultAlphaSize);
#endif
        painter.setFont(sans);

        if (!firstFont && !sufficientSpace(y, &painter, font, sizes, pageHeight, m_size)) {
            m_printer->newPage();
            y = 0;
        }
        painter.setFont(sans);
        y += painter.fontMetrics().height();
        painter.drawText(0, y, name);

        y += constMarginLineBefore;
        painter.drawLine(0, y, pageWidth, y);
        y += constMarginLineAfter;

        bool onlyDrawChars = false;
        Qt::TextElideMode em = Qt::LeftToRight == QApplication::layoutDirection() ? Qt::ElideRight : Qt::ElideLeft;

        if (0 == m_size) {
            font.setPointSize(CFcEngine::constDefaultAlphaSize);
            painter.setFont(font);

            QFontMetrics fm(font, painter.device());
            bool lc = hasStr(font, CFcEngine::getLowercaseLetters()), uc = hasStr(font, CFcEngine::getUppercaseLetters());

            onlyDrawChars = !lc && !uc;

            if (lc || uc) {
                y += CFcEngine::constDefaultAlphaSize;
            }

            if (lc) {
                painter.drawText(0, y, fm.elidedText(CFcEngine::getLowercaseLetters(), em, pageWidth));
                y += constMarginFont + CFcEngine::constDefaultAlphaSize;
            }

            if (uc) {
                painter.drawText(0, y, fm.elidedText(CFcEngine::getUppercaseLetters(), em, pageWidth));
                y += constMarginFont + CFcEngine::constDefaultAlphaSize;
            }

            if (lc || uc) {
                QString validPunc(usableStr(font, CFcEngine::getPunctuation()));
                if (validPunc.length() >= (CFcEngine::getPunctuation().length() / 2)) {
                    painter.drawText(0, y, fm.elidedText(CFcEngine::getPunctuation(), em, pageWidth));
                    y += constMarginFont + constMarginLineBefore;
                }
                painter.drawLine(0, y, pageWidth, y);
                y += constMarginLineAfter;
            }
        }

        for (; sizes[s]; ++s) {
            y += sizes[s];
            font.setPointSize(sizes[s]);
            painter.setFont(font);

            QFontMetrics fm(font, painter.device());

            if (sufficientSpace(y, pageHeight, fm)) {
                painter.drawText(0, y, fm.elidedText(previewString(font, str, onlyDrawChars), em, pageWidth));
                if (sizes[s + 1]) {
                    y += constMarginFont;
                }
            } else {
                break;
            }
        }
        y += (s < 1 || sizes[s - 1] < 25 ? 14 : 28);
        firstFont = false;
    }
    Q_EMIT progress(m_items.count(), QString());
    painter.end();

    //
    // Did we change the users font settings? If so, reset to their previous values...
    if (changedFontEmbeddingSetting) {
        m_printer->setFontEmbeddingEnabled(false);
    }
}

CPrinter::CPrinter(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Print"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CPrinter::slotCancelClicked);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QFrame *page = new QFrame(this);
    QGridLayout *layout = new QGridLayout(page);
    m_statusLabel = new QLabel(page);
    m_progress = new QProgressBar(page);
    layout->addWidget(m_actionLabel = new CActionLabel(this), 0, 0, 2, 1);
    layout->addWidget(m_statusLabel, 0, 1);
    layout->addWidget(m_progress, 1, 1);
    m_progress->setRange(0, 100);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 2, 0);

    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);
    setMinimumSize(420, 80);
}

CPrinter::~CPrinter()
{
}

void CPrinter::print(const QList<Misc::TFont> &items, int size)
{
#ifdef HAVE_LOCALE_H
    char *oldLocale = setlocale(LC_NUMERIC, "C");
#endif

    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, parentWidget());

    if (dialog->exec()) {
        CPrintThread *thread = new CPrintThread(&printer, items, size, this);

        m_progress->setRange(0, items.count());
        m_progress->setValue(0);
        progress(0, QString());
        connect(thread, &CPrintThread::progress, this, &CPrinter::progress);
        connect(thread, &QThread::finished, this, &QDialog::accept);
        connect(this, &CPrinter::cancelled, thread, &CPrintThread::cancel);
        m_actionLabel->startAnimation();
        thread->start();
        exec();
        delete thread;
    }

    delete dialog;

#ifdef HAVE_LOCALE_H
    if (oldLocale) {
        setlocale(LC_NUMERIC, oldLocale);
    }
#endif
}

void CPrinter::progress(int p, const QString &label)
{
    if (!label.isEmpty()) {
        m_statusLabel->setText(label);
    }
    m_progress->setValue(p);
}

void CPrinter::slotCancelClicked()
{
    m_statusLabel->setText(i18n("Canceling…"));
    Q_EMIT cancelled();
}

void CPrinter::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)
    e->ignore();
    slotCancelClicked();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(KFI_CATALOGUE);
    KAboutData aboutData("kfontprint",
                         i18n("Font Printer"),
                         WORKSPACE_VERSION_STRING,
                         i18n("Simple font printer"),
                         KAboutLicense::GPL,
                         i18n("(C) Craig Drummond, 2007"));
    KAboutData::setApplicationData(aboutData);

    QGuiApplication::setWindowIcon(QIcon::fromTheme("kfontprint"));

    QCommandLineParser parser;
    const QCommandLineOption embedOption(QLatin1String("embed"), i18n("Makes the dialog transient for an X app specified by winid"), QLatin1String("winid"));
    parser.addOption(embedOption);
    const QCommandLineOption sizeOption(QLatin1String("size"), i18n("Size index to print fonts"), QLatin1String("index"));
    parser.addOption(sizeOption);
    const QCommandLineOption pfontOption(
        QLatin1String("pfont"),
        i18n("Font to print, specified as \"Family,Style\" where Style is a 24-bit decimal number composed as: <weight><width><slant>"),
        QLatin1String("font"));
    parser.addOption(pfontOption);
    const QCommandLineOption listfileOption(QLatin1String("listfile"), i18n("File containing list of fonts to print"), QLatin1String("file"));
    parser.addOption(listfileOption);
    const QCommandLineOption deletefileOption(QLatin1String("deletefile"), i18n("Remove file containing list of fonts to print"));
    parser.addOption(deletefileOption);

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QList<Misc::TFont> fonts;
    int size(parser.value(sizeOption).toInt());

    if (size > -1 && size < 256) {
        QString listFile(parser.value(listfileOption));

        if (!listFile.isEmpty()) {
            QFile f(listFile);

            if (f.open(QIODevice::ReadOnly)) {
                QTextStream str(&f);

                while (!str.atEnd()) {
                    QString family(str.readLine()), style(str.readLine());

                    if (!family.isEmpty() && !style.isEmpty()) {
                        fonts.append(Misc::TFont(family, style.toUInt()));
                    } else {
                        break;
                    }
                }
                f.close();
            }

            if (parser.isSet(deletefileOption)) {
                ::unlink(listFile.toLocal8Bit().constData());
            }
        } else {
            QStringList fl(parser.values(pfontOption));
            QStringList::ConstIterator it(fl.begin()), end(fl.end());

            for (; it != end; ++it) {
                QString f(*it);

                int commaPos = f.lastIndexOf(',');

                if (-1 != commaPos) {
                    fonts.append(Misc::TFont(f.left(commaPos), f.mid(commaPos + 1).toUInt()));
                }
            }
        }

        if (!fonts.isEmpty()) {
            CPrinter(createParent(parser.value(embedOption).toInt(nullptr, 16))).print(fonts, size);

            return 0;
        }
    }

    return -1;
}
