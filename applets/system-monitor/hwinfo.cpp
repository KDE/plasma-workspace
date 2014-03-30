/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2010 Michel Lafon-Puyo <michel.lafonpuyo@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "hwinfo.h"

#include <Plasma/TextBrowser>
#include <Plasma/IconWidget>
#include <Plasma/Containment>
#include <Plasma/ToolTipManager>
#include <Plasma/Theme>
#include <KTextBrowser>
#include <QDebug>
#include <KStandardDirs>
#include <KIcon>
#include <QGraphicsLinearLayout>

#define START "<html><head><style type=\"text/css\">\
td { vertical-align: top; font-size:7pt; font-weight:normal; font-style:normal; color: %2; } \
</style></head><body>"
#define START_BASIC "<html><head></head><body>"
#define START_TABLE "<table>"
#define INFO_ROW "<tr><td>%1:</td><td>%2</td></tr>"
#define END_TABLE "</table>"
#define END "</body><html>"

HWInfo::HWInfo(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args), m_info(0), m_icon(0)
{
    resize(234 + 20 + 23, 135 + 20 + 25);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateHtml()));
}

HWInfo::~HWInfo()
{
}

void HWInfo::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    setTitle(i18n("Hardware Info"));
    setEngine(dataEngine("soliddevice"));
    setSources();
    connectToEngine();
}

bool HWInfo::addVisualization(const QString& source)
{
    if (mode() != SM::Applet::Panel) {
        if (!m_info) {
            m_info = new Plasma::TextBrowser(this);
            m_info->nativeWidget()->setHtml(QString(START + i18n("Getting hardware information...") + END));
            appendVisualization(source, m_info);
            //m_info->nativeWidget()->document()->setTextWidth(contentsRect().width());
            //setPreferredItemHeight(m_info->nativeWidget()->document()->size().height());
            setPreferredItemHeight(135);
        }
    } else {
        if (!m_icon) {
            m_icon = new Plasma::IconWidget(KIcon(icon()), "", this);
            appendVisualization(source, m_icon);
        }
    }
    return true;
}

void HWInfo::deleteVisualizations()
{
    SM::Applet::deleteVisualizations();
    m_icon = 0;
    m_info = 0;
}

void HWInfo::setSources()
{
    m_cpus = engine()->query("IS Processor")["IS Processor"].toStringList();
    foreach (const QString& id, m_cpus) {
        appendSource(id);
    }
    m_networks = engine()->query("IS NetworkInterface")["IS NetworkInterface"].toStringList();
    foreach (const QString& id, m_networks) {
        appendSource(id);
    }
    m_audios = engine()->query("IS AudioInterface")["IS AudioInterface"].toStringList();
    foreach (const QString& id, m_audios) {
        appendSource(id);
    }
    // TODO: get this from soliddevice
    Plasma::DataEngine* engine = dataEngine("executable");
    QString path = QString::fromLocal8Bit(qgetenv("PATH"))
                 + QString::fromLatin1(":/usr/sbin:/sbin/");
    QString exe = KStandardDirs::findExe( "lspci", path );
    if (exe.isEmpty()) {
       kError()  << "lspci not found in " << path << endl;
    } else {
       QString tmp = exe + " | grep VGA | sed 's/.*: //g'";
       engine->connectSource(tmp, this);
    }
}

void HWInfo::dataUpdated(const QString& source,
                         const Plasma::DataEngine::Data &data)
{
    if (m_audios.contains(source) && !m_audioNames.contains(data["Name"].toString()) &&
        !data["Name"].toString().isEmpty()) {
        m_audioNames.append(data["Name"].toString());
    } else if (m_networks.contains(source) && !m_networkNames.contains(data["Product"].toString()) &&
               !data["Product"].toString().isEmpty()) {
        m_networkNames.append(data["Product"].toString());
    } else if (m_cpus.contains(source) && !m_cpuNames.contains(data["Product"].toString()) &&
               !data["Product"].toString().isEmpty()) {
        m_cpuNames.append(data["Product"].toString().trimmed());
    } else if (source.indexOf("VGA") > -1) {
        m_gpu = data["stdout"].toString().trimmed();
    }
    updateHtml();
}

void HWInfo::updateHtml()
{
    QString html;
    foreach(const QString& cpu, m_cpuNames) {
        html += QString(INFO_ROW).arg(i18n("CPU")).arg(cpu);
    }
    html += QString(INFO_ROW).arg(i18n("GPU")).arg(m_gpu);
    foreach(const QString& audio, m_audioNames) {
        html += QString(INFO_ROW).arg(i18n("Audio")).arg(audio);
    }
    foreach(const QString& network, m_networkNames) {
        html += QString(INFO_ROW).arg(i18n("Network")).arg(network);
    }
    html += END_TABLE END;
    if (m_info) {
        Plasma::Theme* theme = Plasma::Theme::defaultTheme();
        html = QString(START START_TABLE)
                .arg(theme->color(Plasma::Theme::TextColor).name()) + html;
        m_info->nativeWidget()->setHtml(html);
    } else if (m_icon) {
        html = START_BASIC START_TABLE + html;
        Plasma::ToolTipContent data(i18n("Hardware Info"), html);
        Plasma::ToolTipManager::self()->setContent(m_icon, data);
    }
}

#include "hwinfo.moc"
