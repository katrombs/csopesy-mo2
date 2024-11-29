#include "Process.h"
#include "BaseScreen.h"
#include "ConsoleManager.h"
#include "ScheduleWorker.h"
#include "MainConsole.h"
#include "FileWrite.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <ctime>
#include <vector>
#include "MemoryManager.h"
#include <random>

long long MainConsole::delaysPerExec = 0;
int ScheduleWorker::quantumCycleCounter = 0;
long long Process::busyTime = MainConsole::delaysPerExec;
long long maxOverallMem = MainConsole::maxOverallMem;

long long Process::getMemoryUsage() const {
    return endAddress - startAddress;
}

Process::Process(const std::string& name, int id, long long totalLines, const std::string& timeCreated, int coreAssigned, long long startAddress, long long endAddress)
    : processName(name), processID(id), totalLineOfInstruction(totalLines),
    currLineOfInstruction(0), timeCreated(timeCreated), coreAssigned(coreAssigned), startAddress(startAddress), endAddress(endAddress) {}

void Process::incrementLine(int core) {

    if (this != nullptr) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto& unfinishedList = ConsoleManager::getInstance()->unfinishedProcessList;
            if (std::find(unfinishedList.begin(), unfinishedList.end(), this) == unfinishedList.end()) {
                unfinishedList.push_back(this);
            }
        }

        int quantum = (MainConsole::scheduler == "rr") ? (MainConsole::quantumCycles > 0 ? MainConsole::quantumCycles : totalLineOfInstruction) : 1;

        while (currLineOfInstruction < totalLineOfInstruction) {
            {
                std::unique_lock<std::mutex> lock(mtx);

                // wait till clock advances
                while (this->processCurCycle == MainConsole::curClockCycle) {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
                    lock.lock();
                }

                for (int i = 0; i < quantum && currLineOfInstruction < totalLineOfInstruction; ++i) {
                    if (busyTime == 0) {
                        // Get the current time and format it
                        time_t currTime;
                        char timeCreation[50];
                        struct tm datetime;
                        time(&currTime);
                        localtime_s(&datetime, &currTime);
                        strftime(timeCreation, sizeof(timeCreation), "%m/%d/%Y %I:%M:%S %p", &datetime);

                        // Set the core assigned for this process
                        this->setCoreAssigned(core);
                        std::string timeCreated = "Executed line at timestamp: (" + (std::string)timeCreation + ")"; // timestamp of execution
                        std::string coreUsed = "Core: " + std::to_string(coreAssigned); // associated core
                        std::string printExec = "Hello World from " + processName; // Create Print Statement (execution)
                        std::string log = timeCreated + "   " + coreUsed + "   " + printExec;
                        printLogs.push_back(log); // Put print statement to printLogs

                        // Increment the current line of instruction and update the process cycle
                        currLineOfInstruction++;
                        this->busyTime = MainConsole::delaysPerExec;
                    }
                    else {
                        this->busyTime--;
                    }
                    this->processCurCycle = MainConsole::curClockCycle;
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Adjust sleep time as needed
                    lock.lock();
                }
            }
            std::this_thread::yield();
        }

        // if complete after quantum, mark the process as finished 
        if (currLineOfInstruction >= totalLineOfInstruction) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                ScheduleWorker::cores[coreAssigned] = -1; // mark core as avail
                ScheduleWorker::usedCores--;
                ConsoleManager::getInstance()->addFinishedProcess(this);
            }
            freePages();
            MemoryManager::getInstance()->deallocateFlat(startAddress, endAddress - startAddress); // deallocate
        }
    }
}


void Process::freePages() {
    for (auto& entry : pageTable) {
        if (entry.second.valid) {
            MemoryManager::getInstance()->releaseFrame(entry.second.frameNumber);
            entry.second.valid = false;
        }
    }
}

void Process::processSMI(){
    ConsoleManager* consoleManager = ConsoleManager::getInstance();

    int usedCores = ScheduleWorker::usedCores;
    int availableCores = MainConsole::totalNumCores - usedCores;

    int usedMemory = consoleManager->getUsedMemory();
    int maxMemory = MainConsole::maxOverallMem;

    std::cerr << "\n--------------------------------------------\n";
    std::cerr << "| PROCESS-SMI V01.00 Driver Version: 01.00 |\n";
    std::cerr << "--------------------------------------------\n";

    std::cerr << "CPU-Util: " << (usedCores * 100) / MainConsole::totalNumCores << "%\n";
    std::cerr << "Memory Usage: " << usedMemory << "MiB / " << maxMemory << "MiB\n";
    std::cerr << "Memory Util: " << (usedMemory * 100) / maxMemory << "%\n";

    std::cerr << "-------------------------------------------\n";
    std::cerr << "Running Processes and Memory Usage:\n";

    auto unfinishedProcesses = consoleManager->getProcessesInMemory();
    for (const auto& process : unfinishedProcesses) {
        std::cerr << process->getName() << "\t" << process->getMemoryUsage() << "MiB\n";
    }
    std::cerr << "-------------------------------------------\n";
}


std::vector<std::string> Process::getPrintLogs() {
    if (!printLogs.empty()) {
        return printLogs;
    }
    return {};
}

void Process::generateMemorySnapshot(int quantumCycle) {
    std::vector<Process*> processesInMemory = ConsoleManager::getInstance()->getProcessesInMemory();
    // for debugging, can be commented out later
    //if (processesInMemory.empty()) {
    //    std::cout << "No processes in memory during snapshot at quantum cycle " << quantumCycle << "\n";
    //}
    int externalFragmentation = ConsoleManager::getInstance()->calculateExternalFragmentation();  
    FileWrite::generateMemorySnapshot(quantumCycle, processesInMemory, externalFragmentation);
}

void Process::setMemoryRange(long long start, long long memSize) {
    this->startAddress = start;
    this->endAddress = start + memSize;
}

void Process::setMemoryRequired(long long memRequired) {
    this->memoryRequired = memRequired;
}

long long Process::getMemoryRequired() const {
    return memoryRequired;
}

void Process::addPage(int pageIndex) {
    PageTableEntry entry;
    entry.frameNumber = pageIndex;
    entry.valid = true;
    pageTable[pageIndex] = entry;
}

bool Process::isFinished() const {
    return currLineOfInstruction >= totalLineOfInstruction;
}