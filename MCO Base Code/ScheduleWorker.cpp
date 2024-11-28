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

std::mutex ScheduleWorker::schedulerMutex;

// Memory
long long MainConsole::maxOverallMem = 0;
int MainConsole::memPerFrame = 0;


ScheduleWorker::ScheduleWorker() {

}

ScheduleWorker::~ScheduleWorker() {

}

void ScheduleWorker::initialize(int numCores) {

    this->schedulerCurCycle = MainConsole::curClockCycle;

    this->availableCores = numCores;
    this->initializeCores(numCores);

    //Make a thread for the scheduler so it can constantly check for processes
    if (MainConsole::scheduler == "fcfs") {
        std::thread scheduleThread(&ScheduleWorker::scheduleProcess, this);
        //Detach it
        scheduleThread.detach();
    }
    else if (MainConsole::scheduler == "rr") {
        std::thread scheduleThread(&ScheduleWorker::roundRobin, this, MainConsole::quantumCycles);
        //Detach it
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
                if (cores[i] == -1) { // Found available core
                    // Assign core to process
                    coreAssigned = i;
                    //Set core to busy
                    cores[i] = 1;
                    // Add count of used cores
                    usedCores++;
                    // Associate core to currently executed process
                    //Start the Process
                    std::thread processIncrementLine(&Process::incrementLine, processList.front(), coreAssigned);
                    //Separate the thread of the process.
                    processIncrementLine.detach();
                    //Remove the current process in processList <-- processList should be empty again 
                    processList.erase(processList.begin());
                    // Add to processList the process at the top of waitingQueue
                    if (!waitingQueue.empty()) {
                        processList.push_back(waitingQueue.front());
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
    // Pause for a moment (This is necessary so that it will start checking on CPU #0 upon initialized)
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    /* Round robin algorithm
    RR Steps:
        1. Assign to runningProcess the process at the top of the ready queue
        2.1 If process is not completely executed, keep executing until quantumCycleCounter == quantumCycles
        2.2 Else if process is completely executed and quantumCycleCounter < quantumCycles, get the process at
            the top of the ready queue and continue executing from current quantumCycleCounter until quantumCycles
        3. When quantumCycleCounter == quantumCycles
        3.1 If process->currLineOfInstruction < totalLineOfInstruction, move process to the end of ready queue
        4. Repeat steps
    */
    int i = 0;
    this->quantumCycleCounter = 0;
    std::shared_ptr<Process> runningProcess = nullptr;
    // Vector of threads of processes concurrently running
    std::vector<std::thread> rrThreads;
    //Memory
    long long currMemAlloc = 0;
    int memoryBlockLoc = 0;

    while (true) {
        //std::vector<std::thread> rrThreads; // vector of processes to run concurrently
        if (this->schedulerCurCycle != MainConsole::curClockCycle) {

            if (runningRRProcessCount == cores.size()) {
                for (int i = 0; i < runningRRProcessList.size(); i++) {
                    if (runningRRProcessList.at(i).get() != nullptr) {
                        if (runningRRProcessList.at(i)->getCurrentLine() != runningRRProcessList.at(i)->getTotalLines()) {
                            ConsoleManager::getInstance()->waitingProcess(runningRRProcessList.at(i).get());
                            cores[runningRRProcessList.at(i)->getCoreAssigned()] = -1;
                        }
                    }
                }
                MemoryManager::prepareMemoryBlocks();

                usedCores = 0;
                runningRRProcessCount = 0;
            }

            // If all cores are checked, recheck all again.
            if (i == cores.size()) {
                i = 0;
            }

            if (!processList.empty()) {
                if (cores[i] == -1 && this->runningRRProcessCount < cores.size()) { // Found available core
                    //std::lock_guard<std::mutex> lock(schedulerMutex);

                    // Core assignment
                    coreAssigned = i;
                    // Set core to busy
                    cores[i] = 1;

                    runningProcess = processList.front(); // Assign process at the top of ready queue
                    runningRRProcessList.push_back(runningProcess);
                    processList.erase(processList.begin()); // Pop top of ready queue

                    // Memory allocation
                    // memory requirement between min and max memory per process
                    std::random_device rd;
                    std::mt19937_64 gen(rd());  // Use 64-bit version of Mersenne Twister
                    std::uniform_int_distribution<long long> dis(MainConsole::minMemPerProc, MainConsole::maxMemPerProc);
                    long long memRequired = dis(gen);

                    // Check if not exceeding maximum overall memory
                    if (currMemAlloc + memRequired <= MainConsole::maxOverallMem) {
                        this->runningRRProcessCount++;
                        // Check if previous memory blocks are taken
                        long long availableMemBlockAddr = 0.0;

                        for (int i = 0; i < MemoryManager::memoryBlocks.size(); i++) {
                            if (MemoryManager::memoryBlocks.at(i) != -1) {
                                availableMemBlockAddr = MemoryManager::memoryBlocks.at(i);
                                MemoryManager::memoryBlocks.at(i) = -1;
                                memoryBlockLoc = i;
                                break;
                            }
                        }

                        // Assign mem-per-proc amount of memory to runningProcess
                        if (availableMemBlockAddr != 0) {
                            runningProcess->setMemoryRange(availableMemBlockAddr, memoryBlockLoc, memRequired);
                        }

                        // Update currMemAlloc
                        currMemAlloc += memRequired;

                        // Create thread and push into rrThreads vector ??
                        std::thread processIncrementLine(&Process::incrementLine, runningProcess, coreAssigned);
                        processIncrementLine.join();

                        // Process has not finished executing
                        if (runningProcess != nullptr) {
                            if (runningProcess->getCurrentLine() < runningProcess->getTotalLines()) {
                                std::lock_guard<std::mutex> lock(schedulerMutex);
                                // Move process at the end of ready queue
                                processList.push_back(runningProcess);
                            }
                        }
                        // Free allocated memory
                        currMemAlloc -= memRequired;

                    }
                    else { // No memory available, push runningProcess back to processList
                        // Move process at the end of ready queue
                        processList.push_back(runningProcess);
                    }

                    // Add count of used cores
                    //usedCores++;
                }
            }
            i++;
            this->schedulerCurCycle = MainConsole::curClockCycle;
            Sleep(100);
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
        if (ScheduleWorker::schedulerCurCycle != MainConsole::curClockCycle) {

            //std::this_thread::sleep_for(std::chrono::milliseconds(300));

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
                for (int i = 0; i < MainConsole::processesNameList.size(); i++) {
                    if (processName == MainConsole::processesNameList[i]) {
                        isProcessNameAvailable = false;
                        break;
                    }
                }

                if (isProcessNameAvailable) {
                    std::random_device rd;
                    std::mt19937_64 gen(rd());  // Use 64-bit version of Mersenne Twister
                    std::uniform_int_distribution<long long> dis(MainConsole::minimumIns, MainConsole::maximumIns);
                    long long random_value = dis(gen);

                    shared_ptr<Process> process = make_shared<Process>(processName, testProcessID, random_value, timeCreated);

                    MainConsole::processesNameList.push_back(processName); //Para di matake ulit ung name

                    shared_ptr<BaseScreen> baseScreen = make_shared<BaseScreen>(process, processName);
                    ConsoleManager::getInstance()->registerScreen(baseScreen);

                    // Check for available cores
                    for (int i = 0; i < ScheduleWorker::cores.size(); i++) {
                        if (ScheduleWorker::cores[i] == -1) {
                            testAnyAvailableCore = true;
                            break;
                        }
                        else {
                            testAnyAvailableCore = false;
                        }
                    }

                    // addProcess if there is available core
                    if (testAnyAvailableCore) {
                        if (process != nullptr) {
                            ScheduleWorker::addProcess(process);
                        }
                    }
                    else { // add to waiting queue if no available core
                        if (process != nullptr) {
                            ScheduleWorker::addWaitProcess(process);
                        }
                    }

                    testProcessID++;
                    ScheduleWorker::schedulerCurCycle = MainConsole::curClockCycle;
                }
                else {
                    std::cerr << "Screen name " << processName << " already exists. Please use a different name." << std::endl;
                }
            }
        }
    }
    

}