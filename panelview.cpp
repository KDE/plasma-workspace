/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "panelview.h"
#include "shellcorona.h"
#include "panelshadows_p.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QQmlEngine>
#include <QQmlContext>
#include <QTimer>

#include <kactioncollection.h>
#include <kwindowsystem.h>
#include <kwindoweffects.h>

#include <Plasma/Containment>
#include <Plasma/Package>

PanelView::PanelView(ShellCorona *corona, QWindow *parent)
    : PlasmaQuickView(corona, parent),
       m_offset(0),
       m_maxLength(0),
       m_minLength(0),
       m_alignment(Qt::AlignLeft),
       m_corona(corona),
       m_strutsTimer(new QTimer(this)),
       m_visibilityMode(NormalPanel)
{
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus);
    KWindowSystem::setType(winId(), NET::Dock);
    setVisible(false);

    themeChanged();
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelView::themeChanged);

    
    m_positionPaneltimer.setSingleShot(true);
    m_positionPaneltimer.setInterval(150);
    connect(&m_positionPaneltimer, &QTimer::timeout,
            this, [=] () {
                restore();
                positionPanel();
            });

    //Screen management
    connect(this, &QWindow::screenChanged,
            this, &PanelView::positionPanel);
    connect(screen(), &QScreen::geometryChanged,
            this, &PanelView::positionPanel);
    connect(this, &PlasmaQuickView::locationChanged,
            &m_positionPaneltimer, [=] () {
                m_positionPaneltimer.start();
            });
    connect(this, &PlasmaQuickView::containmentChanged,
            this, &PanelView::positionPanel);
    connect(this, &PlasmaQuickView::containmentChanged,
            this, [=] () {
                m_positionPaneltimer.start();
            });



    if (!m_corona->package().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    m_strutsTimer->setSingleShot(true);
    connect(m_strutsTimer, &QTimer::timeout,
            this, &PanelView::updateStruts);
    connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged,
            this, &PanelView::updateStruts);

    setResizeMode(PlasmaQuickView::SizeRootObjectToView);
    qmlRegisterType<QScreen>();
    engine()->rootContext()->setContextProperty("panel", this);
    setSource(QUrl::fromLocalFile(m_corona->package().filePath("views", "Panel.qml")));
    PanelShadows::self()->addWindow(this);
}

PanelView::~PanelView()
{
    if (containment()) {
        config().writeEntry("offset", m_offset);
        config().writeEntry("max", m_maxLength);
        config().writeEntry("min", m_minLength);
        if (formFactor() == Plasma::Types::Vertical) {
            config().writeEntry("length", size().height());
            config().writeEntry("thickness", size().width());
        } else {
            config().writeEntry("length", size().width());
            config().writeEntry("thickness", size().height());
        }
        config().writeEntry("alignment", (int)m_alignment);
        m_corona->requestApplicationConfigSync();
        m_corona->requestApplicationConfigSync();
    }
    PanelShadows::self()->removeWindow(this);
}

KConfigGroup PanelView::config() const
{
    if (!containment()) {
        return KConfigGroup();
    }
    KConfigGroup views(m_corona->applicationConfig(), "PlasmaViews");
    views = KConfigGroup(&views, QString("Panel %1").arg(containment()->id()));

    if (containment()->formFactor() == Plasma::Types::Vertical) {
        return KConfigGroup(&views, "Types::Vertical" + QString::number(screen()->size().height()));
    //treat everything else as horizontal
    } else {
        return KConfigGroup(&views, "Horizontal" + QString::number(screen()->size().width()));
    }
}

void PanelView::maximize()
{
    int length;
    if (containment()->formFactor() == Plasma::Types::Vertical) {
        length = screen()->size().height();
    } else {
        length = screen()->size().width();
    }
    setOffset(0);
    setMinimumLength(length);
    setMaximumLength(length);
}

Qt::Alignment PanelView::alignment() const
{
    return m_alignment;
}

void PanelView::setAlignment(Qt::Alignment alignment)
{
    if (m_alignment == alignment) {
        return;
    }

    m_alignment = alignment;
    config().writeEntry("alignment", (int)m_alignment);
    emit alignmentChanged();
    positionPanel();
}

int PanelView::offset() const
{
    return m_offset;
}

void PanelView::setOffset(int offset)
{
    if (m_offset == offset) {
        return;
    }

    if (formFactor() == Plasma::Types::Vertical) {
        if (offset + m_maxLength > screen()->size().height()) {
            setMaximumLength( -m_offset + screen()->size().height() );
        }
    } else {
        if (offset + m_maxLength > screen()->size().width()) {
            setMaximumLength( -m_offset + screen()->size().width() );
        }
    }

    m_offset = offset;
    config().writeEntry("offset", m_offset);
    positionPanel();
    emit offsetChanged();
    m_corona->requestApplicationConfigSync();
    m_strutsTimer->start(STRUTSTIMERDELAY);
}

int PanelView::thickness() const
{
    return config().readEntry<int>("thickness", 30);
}

void PanelView::setThickness(int value)
{
    if (value == thickness()) {
        return;
    }

    config().writeEntry("thickness", value);
    m_corona->requestApplicationConfigSync();
    positionPanel();
}

int PanelView::length() const
{
    return config().readEntry<int>("length",
            formFactor() == Plasma::Types::Vertical ?
                screen()->size().height() :
                screen()->size().width()
        );
}

void PanelView::setLength(int value)
{
    if (value == length()) {
        return;
    }

    config().writeEntry("length", value);
    m_corona->requestApplicationConfigSync();
    positionPanel();
}

int PanelView::maximumLength() const
{
    return m_maxLength;
}

void PanelView::setMaximumLength(int length)
{
    if (length == m_maxLength) {
        return;
    }

    if (m_minLength > length) {
        setMinimumLength(length);
    }

    if (formFactor() == Plasma::Types::Vertical) {
        setMaximumHeight(length);
    } else {
        setMaximumWidth(length);
    }
    config().writeEntry("maxLength", length);
    m_maxLength = length;
    emit maximumLengthChanged();
    positionPanel();
    m_corona->requestApplicationConfigSync();
}

int PanelView::minimumLength() const
{
    return m_minLength;
}

void PanelView::setMinimumLength(int length)
{
    if (length == m_minLength) {
        return;
    }

    if (m_maxLength < length) {
        setMaximumLength(length);
    }

    if (formFactor() == Plasma::Types::Vertical) {
        setMinimumHeight(length);
    } else {
        setMinimumWidth(length);
    }
    config().writeEntry("minLength", length);
    m_minLength = length;
    positionPanel();
    emit minimumLengthChanged();
    m_corona->requestApplicationConfigSync();
}

void PanelView::setVisibilityMode(PanelView::VisibilityMode mode)
{
    m_visibilityMode = mode;

    if (mode == LetWindowsCover) {
        KWindowSystem::setState(winId(), NET::KeepBelow);
    } else {
        KWindowSystem::clearState(winId(), NET::KeepBelow);
    }
    //life is vastly simpler if we ensure we're visible now
    show();

    disconnect(containment(), SIGNAL(activate()), this, SLOT(unhide()));
    if (!(mode == NormalPanel || mode == WindowsGoBelow)) {
        connect(containment(), SIGNAL(activate()), this, SLOT(unhide()));
    }

    config().writeEntry("panelVisibility", (int)mode);


    updateStruts();

    KWindowSystem::setOnAllDesktops(winId(), true);
}

PanelView::VisibilityMode PanelView::visibilityMode() const
{
    return m_visibilityMode;
}

void PanelView::positionPanel()
{
    if (!containment()) {
        return;
    }

    QScreen *s = screen();
    const int oldThickness = thickness();
    const int oldLength = length();

    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        containment()->setFormFactor(Plasma::Types::Horizontal);

        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->geometry().center().x(), s->geometry().top()) + QPoint(m_offset - size().width()/2, 0));
            break;
        case Qt::AlignRight:
            setPosition(s->geometry().topRight() - QPoint(m_offset + size().width(), 0));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->geometry().topLeft() + QPoint(m_offset, 0));
        }
        break;

    case Plasma::Types::LeftEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);

        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->geometry().left(), s->geometry().center().y()) + QPoint(0, m_offset));
            break;
        case Qt::AlignRight:
            setPosition(s->geometry().bottomLeft() - QPoint(0, m_offset + size().height()));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->geometry().topLeft() + QPoint(0, m_offset));
        }
        break;

    case Plasma::Types::RightEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);

        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->geometry().right(), s->geometry().center().y()) - QPoint(width(), 0) + QPoint(0, m_offset - size().height()/2));
            break;
        case Qt::AlignRight:
            setPosition(s->geometry().bottomRight() - QPoint(width(), 0) - QPoint(0, m_offset + size().height()));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->geometry().topRight() - QPoint(width(), 0) + QPoint(0, m_offset));
        }
        break;

    case Plasma::Types::BottomEdge:
    default:
        containment()->setFormFactor(Plasma::Types::Horizontal);

        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->geometry().center().x(), s->geometry().bottom()) + QPoint(m_offset - size().width()/2, 1));
            break;
        case Qt::AlignRight:
            setPosition(s->geometry().bottomRight() - QPoint(0, height()) - QPoint(m_offset + size().width(), 1));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->geometry().bottomLeft() - QPoint(0, height()) + QPoint(m_offset, 1));
        }
    }
    m_strutsTimer->stop();
    m_strutsTimer->start(STRUTSTIMERDELAY);
    setMinimumSize(QSize(0, 0));
    setMaximumSize(screen()->size());

    if (formFactor() == Plasma::Types::Vertical) {
        resize(thickness(), length());
        setMinimumSize(QSize(thickness(), m_minLength));
        setMaximumSize(QSize(thickness(), m_maxLength));

        emit thicknessChanged();
        emit length();
    } else {
        resize(length(), thickness());
        setMinimumSize(QSize(m_minLength, thickness()));
        setMaximumSize(QSize(m_maxLength, thickness()));

        emit thicknessChanged();
        emit length();
    }
}

void PanelView::restore()
{
    if (!containment()) {
        return;
    }

    setVisible(containment()->isUiReady());
    connect(containment(), &Plasma::Containment::uiReadyChanged,
            this, &PanelView::setVisible);

    connect(containment(), SIGNAL(destroyed(QObject *)),
            this, SLOT(deleteLater()));

    static const int MINSIZE = 10;

    m_offset = config().readEntry<int>("offset", 0);
    if (m_alignment != Qt::AlignCenter) {
        m_offset = qMax(0, m_offset);
    }

    m_alignment = (Qt::Alignment)config().readEntry<int>("alignment", Qt::AlignLeft);

    setMinimumSize(QSize(-1, -1));
    //FIXME: an invalid size doesn't work with QWindows
    setMaximumSize(screen()->size());

    if (containment()->formFactor() == Plasma::Types::Vertical) {
        m_maxLength = config().readEntry<int>("maxLength", screen()->size().height());
        m_minLength = config().readEntry<int>("minLength", screen()->size().height());

        const int maxSize = screen()->size().height() - m_offset;
        m_maxLength = qBound<int>(MINSIZE, m_maxLength, maxSize);
        m_minLength = qBound<int>(MINSIZE, m_minLength, maxSize);

        resize(thickness(), qBound<int>(MINSIZE, length(), maxSize));

        setMinimumHeight(m_minLength);
        setMaximumHeight(m_maxLength);

    //Horizontal
    } else {
        m_maxLength = config().readEntry<int>("maxLength", screen()->size().width());
        m_minLength = config().readEntry<int>("minLength", screen()->size().width());

        const int maxSize = screen()->size().width() - m_offset;
        m_maxLength = qBound<int>(MINSIZE, m_maxLength, maxSize);
        m_minLength = qBound<int>(MINSIZE, m_minLength, maxSize);

        resize(qBound<int>(MINSIZE, length(), maxSize), thickness());

        setMinimumWidth(m_minLength);
        setMaximumWidth(m_maxLength);
    }

    emit maximumLengthChanged();
    emit minimumLengthChanged();
    emit offsetChanged();
    emit alignmentChanged();
}

void PanelView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_panelConfigView) {
        m_panelConfigView.data()->hide();
        m_panelConfigView.data()->deleteLater();
        return;
    }

    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);

    if (cont) {
        m_panelConfigView = new PanelConfigView(cont, this);
    } else {
        m_panelConfigView = new ConfigView(applet);
    }
    m_panelConfigView.data()->init();
    m_panelConfigView.data()->show();
    KWindowSystem::setState(m_panelConfigView.data()->winId(), NET::SkipTaskbar | NET::SkipPager);
}

void PanelView::resizeEvent(QResizeEvent *ev)
{

    if (containment()->formFactor() == Plasma::Types::Vertical) {
        config().writeEntry("length", ev->size().height());
        config().writeEntry("thickness", ev->size().width());
        if (ev->size().height() != ev->oldSize().height()) {
            emit lengthChanged();
        }
        if (ev->size().width() != ev->oldSize().width()) {
            emit thicknessChanged();
        }
    } else {
        config().writeEntry("length", ev->size().width());
        config().writeEntry("thickness", ev->size().height());
        if (ev->size().width() != ev->oldSize().width()) {
            emit lengthChanged();
        }
        if (ev->size().height() != ev->oldSize().height()) {
            emit thicknessChanged();
        }
    }

    PlasmaQuickView::resizeEvent(ev);
}

void PanelView::showEvent(QShowEvent *event)
{
    PanelShadows::self()->addWindow(this);
    PlasmaQuickView::showEvent(event);
    KWindowSystem::setOnAllDesktops(winId(), true);
}

void PanelView::updateStruts()
{
    if (!containment() || !screen()) {
        return;
    }

    NETExtendedStrut strut;

    if (m_visibilityMode == NormalPanel) {
        const QRect thisScreen = corona()->screenGeometry(containment()->screen());
        const QRect wholeScreen = screen()->availableGeometry();

        //Extended struts against a screen edge near to another screen are really harmful, so windows maximized under the panel is a lesser pain
        //TODO: force "windows can cover" in those cases?
        const int numScreens = corona()->numScreens();
        for (int i = 0; i < numScreens; ++i) {
            if (i == containment()->screen()) {
                continue;
            }

            const QRect otherScreen = corona()->screenGeometry(i);

            switch (location())
            {
                case Plasma::Types::TopEdge:
                if (otherScreen.bottom() <= thisScreen.top()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            case Plasma::Types::BottomEdge:
                if (otherScreen.top() >= thisScreen.bottom()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            case Plasma::Types::RightEdge:
                if (otherScreen.left() >= thisScreen.right()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            case Plasma::Types::LeftEdge:
                if (otherScreen.right() <= thisScreen.left()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            default:
                return;
            }
        }
        // extended struts are to the combined screen geoms, not the single screen
        int leftOffset = wholeScreen.x() - thisScreen.x();
        int rightOffset = wholeScreen.right() - thisScreen.right();
        int bottomOffset = wholeScreen.bottom() - thisScreen.bottom();
        int topOffset = wholeScreen.top() - thisScreen.top();
        qDebug() << "screen l/r/b/t offsets are:" << leftOffset << rightOffset << bottomOffset << topOffset << location();

        switch (location())
        {
            case Plasma::Types::TopEdge:
                strut.top_width = height() + topOffset;
                strut.top_start = x();
                strut.top_end = x() + width() - 1;
                break;

            case Plasma::Types::BottomEdge:
                strut.bottom_width = height() + bottomOffset;
                strut.bottom_start = x();
                strut.bottom_end = x() + width() - 1;
                //qDebug() << "setting bottom edge to" << strut.bottom_width
                //         << strut.bottom_start << strut.bottom_end;
                break;

            case Plasma::Types::RightEdge:
                strut.right_width = width() + rightOffset;
                strut.right_start = y();
                strut.right_end = y() + height() - 1;
                break;

            case Plasma::Types::LeftEdge:
                strut.left_width = width() + leftOffset;
                strut.left_start = y();
                strut.left_end = y() + height() - 1;
                break;

            default:
                //qDebug() << "where are we?";
                break;
        }
    }

    KWindowSystem::setExtendedStrut(winId(), strut.left_width,
                                             strut.left_start,
                                             strut.left_end,
                                             strut.right_width,
                                             strut.right_start,
                                             strut.right_end,
                                             strut.top_width,
                                             strut.top_start,
                                             strut.top_end,
                                             strut.bottom_width,
                                             strut.bottom_start,
                                             strut.bottom_end);

    //recreateUnhideTrigger();
}

void PanelView::themeChanged()
{
    //TODO: how to take the shape from the framesvg?
    KWindowEffects::enableBlurBehind(winId(), true);
    KWindowEffects::enableBackgroundContrast(winId(), m_theme.backgroundContrastEnabled(),
                                                      m_theme.backgroundContrast(),
                                                      m_theme.backgroundIntensity(),
                                                      m_theme.backgroundSaturation());
}

#include "moc_panelview.cpp"
