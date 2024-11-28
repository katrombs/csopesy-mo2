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

long long MainConsole::delaysPerExec = 0;
int ScheduleWorker::quantumCycleCounter = 0;
long long Process::busyTime = MainConsole::delaysPerExec;

Process::Process(const std::string& name, int id, long long totalLines, const std::string& timeCreated, int coreAssigned, long long startAddress, long long endAddress)
    : processName(name), processID(id), totalLineOfInstruction(totalLines),
    currLineOfInstruction(0), timeCreated(timeCreated), coreAssigned(coreAssigned), startAddress(startAddress), endAddress(endAddress) {}

void Process::incrementLine(int core) {

    if (this != nullptr) {
        ConsoleManager::getInstance()->unfinishedProcessList.push_back(this);

        while (true) {
            {
                std::lock_guard<std::mutex> lock(mtx);

                // Check if the current process cycle is different from the main clock cycle
                if (this->processCurCycle != MainConsole::curClockCycle) {

                    // Check for busy time if 0
                    if (busyTime == 0) {
                        // Check if the current line of instruction exceeds total instructions
                        if (currLineOfInstruction >= totalLineOfInstruction) {
                            break; // Exit loop if done
                        }

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
                        if (MainConsole::scheduler == "fcfs") {
                            currLineOfInstruction++;
                        }
                        else if (MainConsole::scheduler == "rr") {

                            ScheduleWorker::usedCores++;
                            for (int i = 0; i < MainConsole::quantumCycles; i++) {
                                if (currLineOfInstruction >= totalLineOfInstruction) {
                                    break;
                                }
                                else {
                                    currLineOfInstruction++;
                                }
                            }

                            generateMemorySnapshot(ScheduleWorker::quantumCycleCounter);
                            ScheduleWorker::quantumCycleCounter++;
                            break;
                        }
                        this->processCurCycle = MainConsole::curClockCycle;
                        this->busyTime = MainConsole::delaysPerExec;
                    }
                    else {
                        this->setCoreAssigned(core);

                        // Process in CPU but waits for "delay" to finish before executing
                        this->busyTime--;
                        this->processCurCycle = MainConsole::curClockCycle;

                        // Memory snapshot file for every quantum cycle
                        if (ScheduleWorker::quantumCycleCounter % MainConsole::quantumCycles == 0) {
                            //// ******* Can be commented out later ******* 
                            //std::cout << "Quantum cycle " << ScheduleWorker::quantumCycleCounter
                            //    << " completed. Generating memory snapshot...\n";
                            //// ******************************************
                            generateMemorySnapshot(ScheduleWorker::quantumCycleCounter);
                        }
                    }

                }
            } // Unlocks mtx when going out of scope


        }

        // After exiting the loop, handle the finishing process logic
        {
            std::lock_guard<std::mutex> lock(mtx); // Lock for modifying shared resources
            if (currLineOfInstruction >= totalLineOfInstruction) {
                ScheduleWorker::cores[coreAssigned] = -1; // Mark the core as available
                ConsoleManager::getInstance()->addFinishedProcess(this);
                //FileWrite::generateFile(processID, processName, getTimeCreated(), printLogs);
                //ScheduleWorker::usedCores--;
            }
        }
    }
}

//void Process::processSMI() const {
//    std::cout << "Process Name: " << processName << std::endl;
//    std::cout << "ID: " << processID << std::endl;
//    std::cout << "Current Line of Instruction: " << currLineOfInstruction << "/" << totalLineOfInstruction << std::endl;
//
//    if (currLineOfInstruction >= totalLineOfInstruction) {
//        std::cout << "Status: Finished" << std::endl;
//    }
//    else {
//        std::cout << "Status: Running" << std::endl;
//    }
//}

void Process::processSMI()  {
    ConsoleManager* consoleManager = ConsoleManager::getInstance();

    std::cerr << "\n--------------------------------------------\n";
    std::cerr << "| PROCESS-SMI V01.00 Driver Version: 01.00 |\n";
    std::cerr << "--------------------------------------------\n";

    int usedCores = ScheduleWorker::usedCores;
    int availableCores = MainConsole::totalNumCores - usedCores;

    std::cerr << "CPU-Util: " << (usedCores * 100) / MainConsole::totalNumCores << "%\n";
    std::cerr << "Cores Used: " << usedCores << "\n";
    std::cerr << "Cores Available: " << availableCores << "\n";

    int usedMemory = consoleManager->getUsedMemory();
    int maxMemory = MainConsole::maxOverallMem;
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

long long Process::getMemoryUsage() const {
    return memoryUsage; 
}


void Process::generateMemorySnapshot(int quantumCycle) {
    std::vector<Process*> processesInMemory = ConsoleManager::getInstance()->getProcessesInMemory();
    // for debugging, can be commented out later
    if (processesInMemory.empty()) {
        std::cout << "No processes in memory during snapshot at quantum cycle " << quantumCycle << "\n";
    }
    int externalFragmentation = ConsoleManager::getInstance()->calculateExternalFragmentation(MainConsole::maxOverallMem);
    FileWrite::generateMemorySnapshot(quantumCycle, processesInMemory, externalFragmentation);
}

void Process::setMemoryRange(long long start, int memoryBlockLoc) {

    if (this != nullptr) {
        this->memoryBlockLoc = memoryBlockLoc;
        storedStartAddress = start;
        startAddress = start;
        endAddress = (startAddress - MainConsole::memPerProcess);
        storedEndAddress = endAddress;

        for (int i = 0; i < MemoryManager::memoryBlocks.size(); i++) {
            if (MemoryManager::memoryBlocks.at(i) == endAddress) {
                MemoryManager::memoryBlocks.at(i) = -1;
                break;
            }
        }
        //for (int i = 0; i < MemoryManager::memoryBlocks.size(); i++) {
        //    
        //    std::cerr << MemoryManager::memoryBlocks.at(i) << std::endl;
     
        //}

    }

}