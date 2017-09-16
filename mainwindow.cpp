/*
 * Copyright (C) 2017 Lily Rivers (VioletDarkKitty)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialogue.h"
#include "preferencesdialogue.h"
#include "cSpline.h"
#include <iostream>
#include "colourhelper.h"
using namespace colourHelper;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    settings = new QSettings("system-monitor","system-monitor");

    processesThread = new processInformationWorker(this, settings);
    resourcesThread = new resourcesWorker(this, settings);
    filesystemThread = new fileSystemWorker(this, settings);

    processesThread->start();
    resourcesThread->start();
    filesystemThread->start();

    quitAction = this->findChild<QAction*>("actionQuit");
    connect(quitAction,SIGNAL(triggered(bool)),QApplication::instance(),SLOT(quit()));

    aboutAction = this->findChild<QAction*>("actionAbout");
    connect(aboutAction,SIGNAL(triggered(bool)),this,SLOT(showAboutWindow()));

    preferencesAction = this->findChild<QAction*>("actionPreferences");
    connect(preferencesAction,SIGNAL(triggered(bool)),this,SLOT(showPreferencesWindow()));

    mainTabs = findChild<QTabWidget*>("tabWidgetMain");
    connect(mainTabs, SIGNAL(currentChanged(int)), this, SLOT(handleTabChange()));

    qRegisterMetaType<qcustomplotCpuVector>("qcustomplotCpuVector");
    cpuPlot = reinterpret_cast<QCustomPlot*>(ui->tabResources->findChild<QWidget*>("cpuPlot"));
    connect(resourcesThread,SIGNAL(updateCpuPlotSIG(const qcustomplotCpuVector&)),
            this,SLOT(updateCpuPlotSLO(const qcustomplotCpuVector&)));
    cpuInfoArea = ui->tabResources->findChild<QGridLayout*>("cpuInfoArea");

    qRegisterMetaType<qcustomplotNetworkVector>("qcustomplotNetworkVector");
    networkPlot = reinterpret_cast<QCustomPlot*>(ui->tabResources->findChild<QWidget*>("networkPlot"));
    connect(resourcesThread,SIGNAL(updateNetworkPlotSIG(const qcustomplotNetworkVector&)),
            this,SLOT(updateNetworkPlotSLO(qcustomplotNetworkVector)));

    handleTabChange();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete processesThread;
    delete resourcesThread;
    delete filesystemThread;
    delete mainTabs;
    delete settings;
}

void MainWindow::updateCpuAreaInfo(const QVector<double> &input)
{
    static QVector<QLabel*> cpuLabels;
    static QVector<QPushButton*> cpuColourButtons;
    #define colourNamesLen 4
    static const QString colourNames[] = {
        "orange","red","green","blue"
    };

    if (cpuInfoArea->count() == 0) {
        for(int i=0; i<input.size(); i++) {
            QLabel *cpuLabel = new QLabel(QString::number(input.at(i)));
            cpuLabels.append(cpuLabel);

            QPushButton *cpuColourButton = new QPushButton();
            cpuColourButton->setFlat(true);
            cpuColourButton->setObjectName("CPU" + QString::number(i+1));
            cpuColourButtons.append(cpuColourButton);
            /// TODO: dangarous, across thread and parent may not be correct, move to another object. See below todo
            connect(cpuColourButton, SIGNAL(clicked(bool)), this->resourcesThread, SLOT(createColourDialogue()), Qt::UniqueConnection);

            QHBoxLayout *cpuLayout = new QHBoxLayout();
            cpuLayout->addWidget(cpuColourButton);
            cpuLayout->addWidget(cpuLabel);

            cpuInfoArea->addLayout(cpuLayout, i / 4, i % 4);
            cpuInfoArea->setAlignment(cpuLayout, Qt::AlignHCenter);

            /// TODO: horrible, make a class. Duplicated in resourcesworker
            QColor colour = QColor(colourNames[i % colourNamesLen]);
            colour = colour.toRgb();
            struct__intArrayHolder rgbColor;
            rgbColor.array[0] = colour.red();
            rgbColor.array[1] = colour.green();
            rgbColor.array[2] = colour.blue();
            this->defaultCpuColours["CPU" + QString::number(i+1)] = rgbColor;
        }
        return;
    }

    for(int i=0; i<cpuLabels.size(); i++) {
        cpuLabels[i]->setText("CPU" + QString::number(i+1) + " "
                                + QString::number(memoryConverter::roundDouble(input[i], 1)) + "%");

        QPixmap cpuColour = QPixmap(cpuColourButtons[i]->width(), cpuColourButtons[i]->height());
        cpuColour.fill(createColourFromSettings(settings, cpuColourButtons[i]->objectName(),
                                                      this->defaultCpuColours[cpuColourButtons[i]->objectName()].array));
        cpuColourButtons[i]->setIcon(QIcon(cpuColour));
    }
}

void MainWindow::updateCpuPlotSLO(const qcustomplotCpuVector &input)
{
    QVector<double> x(60); // initialize with entries 60..0
    for (int i=59; i>0; --i)
    {
      x[i] = i;
    }

    const qcustomplotCpuVector *values = &input;

    static bool previouslyPlotted = false;
    int size = values->count();//values->size();
    if (size == 0) {
        return;
    }

    // check if using spline
    QVector<QVector<double>> splineValues;
    QVector<QVector<double>> splineXValues;
    bool smooth = settings->value("smoothGraphs", false).toBool();
    if (smooth) {
        // redo all point positions for y using cSpline
        for(int i=0; i<size; i++) {
            std::vector<double> origX = x.toStdVector();
            std::vector<double> origY = values->at(i).toStdVector();
            raven::cSpline spline(origX,origY);

            if(spline.IsError()!=raven::cSpline::e_error::no_error) {
                std::cerr << "Spline error [cpu] " << spline.IsError() << std::endl;
            }

            QVector<double> splineY, splineX;
            spline.Draw([&splineY, &splineX](double xval, double yval) mutable {
                splineY.append(yval);
                splineX.append(xval);
            },x.length()*5);

            splineValues.push_back(splineY);
            splineXValues.push_back(splineX);
        }

        values = &splineValues;
    }

    QVector<double> lastValues;
    for(int i=0; i<size; i++) {
        lastValues.append(values->at(i).last());
    }
    updateCpuAreaInfo(lastValues);


    for(int i=0; i<size; i++) {
        QString colName = "CPU" + QString::number(i+1);
        QColor cpuColour = createColourFromSettings(settings, colName, this->defaultCpuColours[colName].array);

        if (!previouslyPlotted) {
            cpuPlot->addGraph();
        } else {
            cpuPlot->graph(i)->data()->clear();
            cpuPlot->graph(i)->setPen(QPen(QColor(cpuColour)));
        }
        if (smooth) {
            x = splineXValues[0];
        }
        cpuPlot->graph(i)->setData(x, values->at(i));

        if (settings->value("draw cpu area stacked",false).toBool()) {
            cpuPlot->graph(i)->setBrush(QBrush(cpuColour));
        } else {
            cpuPlot->graph(i)->setBrush(QBrush());
        }
    }
    previouslyPlotted = true;

    if (smooth) {
        cpuPlot->xAxis->setRange(0, x.last() + 1);
    } else {
        cpuPlot->xAxis->setRange(0, values->at(0).size());
    }
    cpuPlot->yAxis->setRange(0, 100);
    cpuPlot->replot();
}

void MainWindow::updateNetworkPlotSLO(const qcustomplotNetworkVector &values)
{
    const memoryConverter *sendingMax = nullptr, *recievingMax = nullptr;

    // find max value to scale all other values by
    for(unsigned int i=0; i<2; i++) {
        for(int j=0; j<values.at(i).size(); j++) {
            switch(i) {
                case 0:
                if (recievingMax == nullptr || (*recievingMax) < values.at(i).at(j)) {
                    recievingMax = &(values.at(i).at(j));
                }
                break;

                case 1:
                if (sendingMax == nullptr || (*sendingMax) < values.at(i).at(j)) {
                    sendingMax = &(values.at(i).at(j));
                }
                break;
            }
        }
    }

    // scale values to the same unit and then plot
    memoryConverter scaler;
    if ((*sendingMax) < (*recievingMax)) {
        scaler = *recievingMax;
    } else {
        scaler = *sendingMax;
    }

    QVector<QVector<double>> scaled, *plotting = &scaled;

    for(unsigned int i=0; i<2; i++) {
        QVector<double> scaledValuesTemp;
        for(int j=0; j<values.at(i).size(); j++) {
            memoryConverter temp = memoryConverter(values.at(i).at(j));
            temp.convertTo(scaler.getUnit());
            scaledValuesTemp.push_back(temp.getValue());
        }
        scaled.push_back(scaledValuesTemp);
    }

    QVector<double> x(60); // initialize with entries 60..0
    for (int i=59; i>0; --i)
    {
      x[i] = i;
    }

    // check if using spline
    QVector<QVector<double>> splineValues;
    QVector<QVector<double>> splineXValues;
    bool smooth = settings->value("smoothGraphs", false).toBool();
    if (smooth) {
        // redo all point positions for y using cSpline
        for(int i=0; i<2; i++) {
            if (scaled.at(i).empty()) {
                continue;
            }

            std::vector<double> origX = x.toStdVector();
            std::vector<double> origY = scaled.at(i).toStdVector();
            raven::cSpline spline(origX,origY);

            if(spline.IsError()!=raven::cSpline::e_error::no_error) {
                std::cerr << "Spline error [network] " << spline.IsError() << std::endl;
            }

            QVector<double> splineY, splineX;

            spline.Draw([&splineY, &splineX](double xval, double yval) mutable {
                splineY.append(yval);
                splineX.append(xval);
            },x.length()*5);

            splineValues.push_back(splineY);
            splineXValues.push_back(splineX);
        }

        plotting = &splineValues;
    }

    QString colours[] = {
        "networkRecievingColourButton",
        "networkSendingColourButton"
    };
    for(unsigned int i=0; i<2; i++) {
        networkPlot->addGraph();
        networkPlot->graph(i)->setPen(QPen(colourHelper::createColourFromSettings(settings, colours[i],
                                                                                  resourcesThread->getColourDefaults()[colours[i]].array)));
        if (smooth) {
            x = splineXValues[0];
        }
        networkPlot->graph(i)->setData(x, plotting->at(i));
    }

    if (smooth) {
        networkPlot->xAxis->setRange(0, x.last() + 1);
    } else {
        networkPlot->xAxis->setRange(0, plotting->at(0).size());
    }
    networkPlot->yAxis->setRange(0, scaler.getValue() + 1);
    networkPlot->yAxis->setLabel(QString::fromStdString(scaler.getUnitAsString()));
    networkPlot->replot();
}

/**
 * @brief MainWindow::handleTabChange Handle when the tab is changed, pause the other threads
 */
void MainWindow::handleTabChange()
{
    unsigned int index = mainTabs->currentIndex();
    processesThread->setPaused(true);
    if (!settings->value("resourcesKeepThreadRunning", true).toBool()) {
        resourcesThread->setPaused(true);
    } else {
        resourcesThread->setPaused(false);
    }
    filesystemThread->setPaused(true);
    switch(index) {
        case 0:
            processesThread->setPaused(false);
        break;

        case 1:
            resourcesThread->setPaused(false);
        break;

        case 2:
            filesystemThread->setPaused(false);
        break;
    }
}

/**
 * @brief MainWindow::showAboutWindow Show the about dialogue without blocking the main thread
 */
void MainWindow::showAboutWindow()
{
    AboutDialogue* about = new AboutDialogue(this);
    //about->show();
    about->exec();
}

/**
 * @brief MainWindow::showPreferencesWindow Show the preferences dialogue
 */
void MainWindow::showPreferencesWindow()
{
    PreferencesDialogue* pref = new PreferencesDialogue(this, settings);
    pref->exec();
}
