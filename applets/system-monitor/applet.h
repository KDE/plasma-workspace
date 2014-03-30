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

#ifndef SM_APPLET_HEADER
#define SM_APPLET_HEADER

#include <Plasma/Applet>

#include "sm_export.h"

#define MINIMUM 24
#define PREFERRED 200

class QGraphicsLinearLayout;
class QGraphicsWidget;

namespace Plasma {
    class Frame;
    class IconWidget;
}

namespace SM {

class SM_EXPORT Applet : public Plasma::Applet
{
    Q_OBJECT
    public:
        enum Mode { Monitor, Panel, Desktop };
        enum Detail { High, Low };

        Applet(QObject *parent, const QVariantList &args);
        ~Applet();

        virtual void constraintsEvent(Plasma::Constraints constraints);
        void save(KConfigGroup &config) const;
        void saveConfig(KConfigGroup &config);

    public Q_SLOTS:
        void toolTipAboutToShow();
        void visualizationDestroyed(QObject *visualization);

    Q_SIGNALS:
        void geometryChecked();

    protected:
        qreal preferredItemHeight() { return m_preferredItemHeight; };
        void setPreferredItemHeight(qreal preferredItemHeight)
                { m_preferredItemHeight = preferredItemHeight; };
        QStringList sources() { return m_sources; };
        void appendSource(const QString& source) { m_sources.append(source); };
        void setSources(const QStringList& sources) { m_sources = sources; };
        void clearSources() { m_sources.clear(); };
        void clear();

        KConfigGroup config();
        void configureLayout();
        void removeLayout();
        void connectToEngine();
        void connectSource(const QString& source);
        QStringList connectedSources();
        void disconnectSources();
        void checkGeometry();
        QGraphicsLinearLayout* mainLayout();
        Plasma::DataEngine* engine();
        void setEngine(Plasma::DataEngine* engine);
        uint interval();
        void setInterval(uint interval);
        QString title();
        void setTitle(const QString& title);
        QHash<QString, QString> tooltips() const;
        void setToolTip(const QString &source, const QString &tipContent);
        Mode mode();
        QGraphicsWidget* visualization(const QString& source);
        virtual bool addVisualization(const QString& source);
        void appendVisualization(const QString& source, QGraphicsWidget *visualization);
        virtual void deleteVisualizations();
        void displayNoAvailableSources();
        virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    private:
        uint m_interval;
        qreal m_preferredItemHeight;
        QString m_title;
        Plasma::Frame* m_header;
        QStringList m_sources;
        QStringList m_connectedSources;
        Plasma::DataEngine *m_engine;
        QHash<QString, QWeakPointer<QGraphicsWidget> > m_visualizations;
        QHash<QString, QString> m_toolTips;
        Qt::Orientation m_orientation;
        Plasma::IconWidget *m_noSourcesIcon;
        Mode m_mode;
        QGraphicsLinearLayout *m_mainLayout;
        Plasma::Applet *m_configSource;
};

}

#endif
