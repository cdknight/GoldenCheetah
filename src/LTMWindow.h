/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_LTMWindow_h
#define _GC_LTMWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"
#include "MetricAggregator.h"
#include "Season.h"
#include "LTMPlot.h"
#include "LTMPopup.h"
#include "LTMTool.h"
#include "LTMSettings.h"
#include "LTMCanvasPicker.h"
#include "GcPane.h"

#include <math.h>

#include <qwt_plot_picker.h>
#include <qwt_text_engine.h>

// track the cursor and display the value for the chosen axis
class LTMToolTip : public QwtPlotPicker
{
    public:
    LTMToolTip(int xaxis, int yaxis, int sflags,
                RubberBand rb, DisplayMode dm, QwtPlotCanvas *pc, QString fmt) :
                QwtPlotPicker(xaxis, yaxis, sflags, rb, dm, pc),
                format(fmt) {}
    virtual QwtText trackerText(const QwtDoublePoint &/*pos*/) const
    {
        QColor bg = QColor(255,255, 170); // toolyip yellow
#if QT_VERSION >= 0x040300
        bg.setAlpha(200);
#endif
        QwtText text;
        QFont def;
        //def.setPointSize(8); // too small on low res displays (Mac)
        //double val = ceil(pos.y()*100) / 100; // round to 2 decimal place
        //text.setText(QString("%1 %2").arg(val).arg(format), QwtText::PlainText);
        text.setText(tip);
        text.setFont(def);
        text.setBackgroundBrush( QBrush( bg ));
        text.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
        return text;
    }
    void setFormat(QString fmt) { format = fmt; }
    void setText(QString txt) { tip = txt; }
    private:
    QString format;
    QString tip;
};

class LTMPlotContainer : public GcWindow
{
    public:
        LTMPlotContainer(QWidget *parent) : GcWindow(parent) {}
        virtual LTMToolTip *toolTip() = 0;
        virtual void pointClicked(QwtPlotCurve *, int) = 0;
        MainWindow *main;
};

class LTMWindow : public LTMPlotContainer
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int chart READ chart WRITE setChart USER true) //XXX hack for now (chart list can change!)
    Q_PROPERTY(int bin READ bin WRITE setBin USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)
    Q_PROPERTY(bool legend READ legend WRITE setLegend USER true)
    Q_PROPERTY(QString dateRange READ dateRange WRITE setDateRange USER true)
    Q_PROPERTY(LTMSettings settings READ getSettings WRITE applySettings USER true)

    public:

        LTMWindow(MainWindow *, bool, const QDir &);
        ~LTMWindow();
        LTMToolTip *toolTip() { return picker; }

        // get/set properties
        int chart() const { return presetPicker->currentIndex(); }
        void setChart(int x) { presetPicker->setCurrentIndex(x); }
        int bin() const { return groupBy->currentIndex(); }
        void setBin(int x) { groupBy->setCurrentIndex(x); }
        bool shade() const { return shadeZones->isChecked(); }
        void setShade(bool x) { shadeZones->setChecked(x); }
        bool legend() const { return showLegend->isChecked(); }
        void setLegend(bool x) { showLegend->setChecked(x); }

        // date ranges set/get the string from the treeWidget
        QString dateRange() const;
        void setDateRange(QString x);

        LTMSettings getSettings() const { return settings; }
        void applySettings(LTMSettings x) { ltmTool->applySettings(&x); }

    public slots:
        void rideSelected();
        void refreshPlot();
        void dateRangeSelected(const Season *);
        void metricSelected();
        void groupBySelected(int);
        void shadeZonesClicked(int);
        void showLegendClicked(int);
        void chartSelected(int);
        void saveClicked();
        void manageClicked();
        void refresh();
        void pointClicked(QwtPlotCurve*, int);
        int groupForDate(QDate, int);

    private:
        // passed from MainWindow
        QDir home;
        bool useMetricUnits;

        // qwt picker
        LTMToolTip *picker;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover

        // popup - the GcPane to display within
        //         and the LTMPopup contents widdget
        GcPane *popup;
        LTMPopup *ltmPopup;

        // preset charts
        QList<LTMSettings> presets;

        // local state
        bool active;
        bool dirty;
        LTMSettings settings; // all the plot settings
        QList<SummaryMetrics> results;
        QList<SummaryMetrics> measures;

        // Widgets
        QSplitter *ltmSplitter;
        LTMPlot *ltmPlot;
        QwtPlotZoomer *ltmZoomer;
        LTMTool *ltmTool;
        QComboBox *presetPicker;
        QComboBox *groupBy;
        QCheckBox *shadeZones;
        QCheckBox *showLegend;
        QPushButton *saveButton;
        QPushButton *manageButton;
};

#endif // _GC_LTMWindow_h
