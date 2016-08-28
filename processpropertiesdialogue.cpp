#include "processpropertiesdialogue.h"
#include "ui_processpropertiesdialogue.h"
#include "tablememoryitem.h"
#include "memoryconversion.h"

processPropertiesDialogue::processPropertiesDialogue(QWidget *parent, pid_t pid) :
    QDialog(parent),
    ui(new Ui::processPropertiesDialogue)
{
    ui->setupUi(this);

    processID = pid;

    processInfoTable = this->findChild<QTableWidget*>("processInfoTable");
    processInfoTable->verticalHeader()->setHidden(true);
    processInfoTable->horizontalHeader()->setHidden(true);

    processInfoTable->setRowCount(8);
    processInfoTable->setColumnCount(2);
    processInfoTable->setItem(processName,0,new QTableWidgetItem("Process Name"));
    processInfoTable->setItem(processUser,0,new QTableWidgetItem("User"));
    processInfoTable->setItem(processStatus,0,new QTableWidgetItem("Status"));
    processInfoTable->setItem(processMemory,0,new QTableWidgetItem("Memory"));
    processInfoTable->setItem(processVirtual,0,new QTableWidgetItem("Virtual Memory"));
    processInfoTable->setItem(processResident,0,new QTableWidgetItem("Resident Memory"));
    processInfoTable->setItem(processShared,0,new QTableWidgetItem("Shared Memory"));
    processInfoTable->setItem(processCPU,0,new QTableWidgetItem("CPU%"));

    connect(this, SIGNAL(updateTable()), this, SLOT(updateTableData()));
    connect(parent,SIGNAL(destroyed(QObject*)),this,SLOT(deleteLater()));

    this->start();
}

processPropertiesDialogue::~processPropertiesDialogue()
{
    delete ui;
    this->quit();
    while(this->running()) {
        ; // wait for the thread to quit
    }
}

void processPropertiesDialogue::updateTableData()
{
    PROCTAB *tab = openproc(PROC_PID | PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLNS);
    proc_t* p; // Don't fill the proc_t struct with any info

    tab->pids = (pid_t*)malloc(sizeof(pid_t*)*2); // PROCTAB should clean this up when closeproc is called
    tab->pids[0] = processID;
    tab->pids[1] = 0; // The array should be 0 terminated

    processInfoTable->setUpdatesEnabled(false);
    if ((p=readproc(tab,NULL))==NULL) {
        // The process is dead
        // GSM shows N/A for some fields in a dead process
    } else {
        // set the fields for the process properties
        processInfoTable->setItem(processName,1,new QTableWidgetItem(p->cmd));
        processInfoTable->setItem(processUser,1,new QTableWidgetItem(p->euser));
        QString status;
        switch(p->state) {
            case 'S':
                status = "Sleeping";
            break;

            case 'R':
                status = "Running";
            break;

            case 'Z':
                status = "Zombie";
            break;
        }
        processInfoTable->setItem(processStatus,1,new QTableWidgetItem(status));
        memoryEntry memory = convertMemoryUnit((p->resident - p->share)*sysconf(_SC_PAGESIZE),memoryUnit::b);
        processInfoTable->setItem(processMemory,1,new TableMemoryItem(memory.unit,truncateDouble(memory.id,1)));
        memoryEntry virtualMemory = convertMemoryUnit(p->vm_size,memoryUnit::kb);
        processInfoTable->setItem(processVirtual,1,new TableMemoryItem(virtualMemory.unit,truncateDouble(virtualMemory.id,1)));
        memoryEntry residentMemory = convertMemoryUnit(p->vm_rss,memoryUnit::kb);
        processInfoTable->setItem(processResident,1,new TableMemoryItem(residentMemory.unit,truncateDouble(residentMemory.id,1)));
        memoryEntry sharedMemory = convertMemoryUnit(p->share*sysconf(_SC_PAGESIZE),memoryUnit::kb);
        processInfoTable->setItem(processShared,1,new TableMemoryItem(sharedMemory.unit,truncateDouble(sharedMemory.id,1)));
        processInfoTable->setItem(processCPU,1,new QTableWidgetItem(0));
    }
    processInfoTable->resizeColumnsToContents();
    processInfoTable->setUpdatesEnabled(true);

    closeproc(tab);
}

void processPropertiesDialogue::loop()
{
    emit(updateTable());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
