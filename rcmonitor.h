/*
 * Copyright (C) 2017 Lily Rivers (VioletDarkKitty)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 3
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef RCMONITOR_H
#define RCMONITOR_H

#include <QWidget>
#include "resourcesworker.h"
#include "filesystemworker.h"
#include <QAction>
#include "qcustomplot.h"
#include <QSettings>
#include <utility>

namespace Ui
{
class RCMonitor;
}

class RCMonitor : public QWidget
{
    Q_OBJECT

public:
    Ui::RCMonitor *ui;
    explicit RCMonitor(QWidget *parent = 0);
    ~RCMonitor();

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void updateCpuPlotSLO(const qcustomplotCpuVector &input);
    void updateNetworkPlotSLO(const qcustomplotNetworkVector &values);

    void updateMemoryPlotSLO(const qcustomplotCpuVector &input);


private slots:
    void handleTabChange();

private:
    QVector<double> x;
    resourcesWorker *resourcesThread;
    fileSystemWorker *filesystemThread;
    QTabWidget *mainTabs;
    QCustomPlot *cpuPlot, *networkPlot, *memoryPlot;
    QSettings *settings;
    QGridLayout *cpuInfoArea;
    void updateCpuAreaInfo(const QVector<double> &input);
    QHash<QString, struct__intArrayHolder> defaultCpuColours; /// TODO: use qcolor?
    QPair<QVector<QVector<double>>, qcustomplotCpuVector> generateSpline(QString name, QVector<double> &x, const QVector<QVector<double>> &y, bool setMax = false);
};

#endif // RCMONITOR_H
