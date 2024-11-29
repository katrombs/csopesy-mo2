#include "ScheduleWorker.h"
#include "Process.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <Windows.h>
#include "MainConsole.h"
#include <random>
#include <condition_variable>
#include "MemoryManager.h"

using namespace std;

int testProcessID = 0; // This could be generated dynamically

std::vector<std::shared_ptr<Process>> ScheduleWorker::processList;
std::vector<std::shared_ptr<Process>> ScheduleWorker::waitingQueue;

long long MainConsole::minimumIns = 0;
long long MainConsole::maximumIns = 0;
std::vector <std::string> MainConsole::processesNameList;
bool testAnyAvailableCore = false;
long long MainConsole::batchProcessFreq = 0;
long long MainConsole::quantumCycles = 0;
String MainConsole::scheduler = "";

int ScheduleWorker::runningRRProcessCount = 0;
std::vector<std::shared_ptr<Process>> ScheduleWorker::runningRRProcessList;
std::atomic<int> ScheduleWorker::usedCores{ 0 };

std::mutex ScheduleWorker::schedulerMutex;
std::mutex ScheduleWorker::runningProcessesMutex;

// Memory
long long MainConsole::maxOverallMem;
int MainConsole::memPerFrame = 0;


ScheduleWorker::ScheduleWorker() {

}

ScheduleWorker::~ScheduleWorker() {

}

void ScheduleWorker::initialize(int numCores) {
    static bool memoryManagerInitialized = false;
    if (!memoryManagerInitialized) {
        MemoryManager::getInstance()->initialize(MainConsole::maxOverallMem, MainConsole::memPerFrame);
        memoryManagerInitialized = true;
    }

    this->schedulerCurCycle = MainConsole::curClockCycle;

    this->availableCores = numCores;
    this->initializeCores(numCores);

    if (MainConsole::scheduler == "fcfs") {
        std::thread scheduleThread(&ScheduleWorker::scheduleProcess, this);
        scheduleThread.detach();
    }
    else if (MainConsole::scheduler == "rr") {
        std::thread scheduleThread(&ScheduleWorker::roundRobin, this, MainConsole::quantumCycles);
        scheduleThread.detach();
    }
}

void ScheduleWorker::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    //for (const auto& existingProcess : processList) {
    //    if (existingProcess->getName() == process->getName()) {
    //        return; // to skip adding the duplicate process
    //    }
    //}
    processList.push_back(process);
    //std::cout << "[DEBUG] Process " << process->getName() << " added to processList." << std::endl;
}

void ScheduleWorker::addWaitProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    waitingQueue.push_back(process);
}

void ScheduleWorker::scheduleProcess() {

    // Pause for a moment (This is necessary so that it will start checking on CPU #0 upon initialized)
    //std::this_thread::sleep_for(std::chrono::milliseconds(300));

    //First-Come First Serve Algorithm
    int i = 0;
    while (true) {

        if (this->schedulerCurCycle != MainConsole::curClockCycle) {
            // If all cores are checked, recheck all again.
            if (i == cores.size()) {
                i = 0;
            }
            if (!processList.empty()) {
                if (cores[i] == -1) { // available core
                    auto process = processList.front();

                    // Attempt memory allocation for the process
                    long long startAddress;
                    if (MemoryManager::getInstance()->allocateFlat(process->getMemoryRequired(), startAddress)) {
                        // Set memory range for the process
                        process->setMemoryRange(startAddress, process->getMemoryRequired());

                        // Assign core to process
                        coreAssigned = i;
                        //Set core to busy
                        cores[i] = 1;
                        // Add count of used cores
                        usedCores++;
                        //Start the Process
                        std::thread processIncrementLine(&Process::incrementLine, process, coreAssigned);
                        // Separate the thread of the process.
                        processIncrementLine.detach();
                        // Remove the current process in processList
                        processList.erase(processList.begin());
                        // Add to processList the process at the top of waitingQueue
                        if (!waitingQueue.empty()) {
                            processList.push_back(waitingQueue.front());
                            waitingQueue.erase(waitingQueue.begin());
                        }
                    }
                    else {
                        // If memory allocation failed, move the process to the waiting queue
                        waitingQueue.push_back(processList.front());
                        processList.erase(processList.begin());
                    }
                }
            }

            i++;
            this->schedulerCurCycle = MainConsole::curClockCycle;
            Sleep(100);
        }

    }
}
void ScheduleWorker::roundRobin(int quantumCycles) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    int i = 0;
    this->quantumCycleCounter = 0;

    while (true) {
        if (this->schedulerCurCycle != MainConsole::curClockCycle) {
            if (i >= cores.size()) {
                i = 0;
            }

            if (!processList.empty()) {
                if (cores[i] == -1) {
                    std::shared_ptr<Process> runningProcess;

                    {
                        std::lock_guard<std::mutex> lock(schedulerMutex);
                        runningProcess = processList.front();
                        processList.erase(processList.begin());
                    }

                    // Assign core to process
                    cores[i] = i;
                    runningProcess->setCoreAssigned(i);
                    usedCores++;

                    // memory allocation using min and max memory per process
                    std::random_device rd;
                    std::mt19937_64 gen(rd());
                    std::uniform_int_distribution<long long> dis(MainConsole::minMemPerProc, MainConsole::maxMemPerProc);
                    long long memRequired = dis(gen);
                    runningProcess->setMemoryRequired(memRequired);

                    long long startAddress = 0;
                    if (MemoryManager::getInstance()->allocateFlat(memRequired, startAddress)) {
                        runningProcess->setMemoryRange(startAddress, memRequired);
                        //std::cout << "[Scheduler] Allocated memory for process " << runningProcess->getName() << std::endl;
                    }
                    else {
                        // memory allocation failed
                        waitingQueue.push_back(runningProcess);
                        //std::cout << "[Scheduler] Memory allocation failed for process " << runningProcess->getName() << std::endl;
                        cores[i] = -1;
                        usedCores--;
                        {
                            std::lock_guard<std::mutex> lock(schedulerMutex);
                            processList.push_back(runningProcess);
                        }
                        i++;
                        continue;
                    }

                    std::thread processThread(&Process::incrementLine, runningProcess, i);
                    processThread.detach();

                    {
                        std::lock_guard<std::mutex> lock(runningProcessesMutex);
                        runningRRProcessList.push_back(runningProcess);
                    }
                }
            }

            {
                std::lock_guard<std::mutex> lock(runningProcessesMutex);
                auto it = runningRRProcessList.begin();
                while (it != runningRRProcessList.end()) {
                    auto process = *it;
                    if (process->isFinished()) {
                        //std::cout << "[Scheduler] Process " << process->getName() << " finished" << std::endl;
                        int coreIndex = process->getCoreAssigned();
                        cores[coreIndex] = -1;
                        usedCores--;
                        it = runningRRProcessList.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
            i++;
            this->schedulerCurCycle = MainConsole::curClockCycle;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void ScheduleWorker::initializeCores(int numCores) {
    for (int i = 0; i < numCores; i++) {
        cores.push_back(-1);
    }
}

void ScheduleWorker::displaySchedule() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    std::cout << "Scheduled Processes:" << std::endl;
    for (const auto& process : processList) {
        std::cout << " - " << process->processName << std::endl;
    }
}

void ScheduleWorker::testSchedule() {
    long long createProcessFreq = MainConsole::batchProcessFreq;

    while (!stopTest) {
        for (long long i = 0; i < createProcessFreq; i++) {
            time_t currTime;
            char timeCreation[50];
            struct tm datetime;
            time(&currTime);
            localtime_s(&datetime, &currTime);
            strftime(timeCreation, sizeof(timeCreation), "%m/%d/%Y %I:%M:%S%p", &datetime);

                std::string timeCreated = (string)timeCreation;
            std::string processName = "autogen_process" + std::to_string(testProcessID);

            bool isProcessNameAvailable = true;
            for (const auto& existingName : MainConsole::processesNameList) {
                if (processName == existingName) {
                    isProcessNameAvailable = false;
                    break;
                }
            }

            if (isProcessNameAvailable) {
                std::random_device rd;
                std::mt19937_64 gen(rd());
                std::uniform_int_distribution<long long> dis(MainConsole::minMemPerProc, MainConsole::maxMemPerProc);
                long long random_value = dis(gen);


                std::shared_ptr<Process> process = std::make_shared<Process>(processName, testProcessID, random_value, timeCreated);
                MainConsole::processesNameList.push_back(processName);

                std::shared_ptr<BaseScreen> baseScreen = std::make_shared<BaseScreen>(process, processName);
                ConsoleManager::getInstance()->registerScreen(baseScreen);

                //std::cout << "[DEBUG] Created process: " << process->getName() << " with " << random_value << " instructions." << std::endl;
                ScheduleWorker::addProcess(process);
                testProcessID++;
            }
            //else {
            //    std::cerr << "Screen name " << processName << " already exists. Please use a different name." << std::endl;
            //}
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }
}


bool ScheduleWorker::allocatePagedMemory(std::shared_ptr<Process> process) {
    int frameSize = MemoryManager::getInstance()->getFrameSize();
    int numPages = (process->getMemoryUsage() + frameSize - 1) / frameSize;
    bool allocationSuccessful = true;

    for (int i = 0; i < numPages; ++i) {
        int pageIndex = MemoryManager::getInstance()->allocatePage();
        if (pageIndex != -1) {
            process->addPage(pageIndex);
        }
        else {
            allocationSuccessful = false;
            break;
        }
    }
    if (!allocationSuccessful) {
        process->freePages();
    }
    return allocationSuccessful;
}


void ScheduleWorker::allocateMemoryForProcess(std::shared_ptr<Process> process) {
    if (allocateFlatMemory(process)) {
        std::cout << "Allocated flat memory for process: " << process->getName() << std::endl;
    }
    else if (allocatePagedMemory(process)) {
        std::cout << "Allocated paged memory for process: " << process->getName() << std::endl;
    }
    else {
        std::cerr << "Memory allocation failed for process: " << process->getName() << std::endl;
        waitingQueue.push_back(process);  // requeue process
    }
}

bool ScheduleWorker::allocateFlatMemory(std::shared_ptr<Process> process) {
    long long memRequired = process->getMemoryUsage();
    long long startAddress;

    if (MemoryManager::getInstance()->allocateFlat(memRequired, startAddress)) {
        process->setMemoryRange(startAddress, memRequired);
        return true;
    }
    return false;
}