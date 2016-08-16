#include "resourcesworker.h"
#include <proc/sysinfo.h>
#include <iostream>
#include <string>
#include "memoryconversion.h"
using namespace memoryConversion;

resourcesWorker::resourcesWorker(QObject *parent)
    : QObject(parent), workerThread()
{
    memoryBar = parent->findChild<QProgressBar*>("memoryBar");
    memoryLabel = parent->findChild<QLabel*>("memoryLabel");
    swapBar = parent->findChild<QProgressBar*>("swapBar");
    swapLabel = parent->findChild<QLabel*>("swapLabel");
}

resourcesWorker::~resourcesWorker()
{
    delete memoryBar;
    delete memoryLabel;
    delete swapBar;
    delete swapLabel;
}

void resourcesWorker::loop()
{
    meminfo(); // have procps read the memory
    //std::cout << kb_main_used << "/" << kb_main_total << std::endl;
    //std::cout << memory << "%" << std::endl;

    /** MEMORY **/
    double memory = ((double)kb_main_used / kb_main_total) * 100;
    memoryBar->setValue(memory);
    memoryEntry mainUsed = convertMemoryUnit(kb_main_used,memoryUnit::kb);
    memoryEntry mainTotal = convertMemoryUnit(kb_main_total,memoryUnit::kb);

    // cleanup the memory values
    std::string mainUsedValue =  dbl2str(truncateDouble(mainUsed.id,1));
    std::string mainTotalValue = dbl2str(truncateDouble(mainTotal.id,1));
    std::string memPercent = dbl2str(truncateDouble(memory,1));

    std::string memoryText = mainUsedValue + unitToString(mainUsed.unit)
            + " (" + memPercent + "%) of " + mainTotalValue + unitToString(mainTotal.unit);
    memoryLabel->setText(memoryText.c_str());

    /** SWAP **/
    double swap = ((double)kb_swap_used / kb_swap_total) * 100;
    swapBar->setValue(swap);
    memoryEntry swapUsed = convertMemoryUnit(kb_swap_used,memoryUnit::kb);
    memoryEntry swapTotal = convertMemoryUnit(kb_swap_total,memoryUnit::kb);

    // cleanup the swap values
    std::string swapUsedValue = dbl2str(truncateDouble(swapUsed.id,1));
    std::string swapTotalValue = dbl2str(truncateDouble(swapTotal.id,1));
    std::string swapPercent = dbl2str(truncateDouble(swap,1));

    std::string swapText = swapUsedValue + unitToString(swapUsed.unit)
            + " (" + swapPercent + "%) of " + swapTotalValue + unitToString(swapTotal.unit);
    swapLabel->setText(swapText.c_str());
}