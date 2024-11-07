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

long long MainConsole::delaysPerExec = 0;
int ScheduleWorker::quantumCycleCounter = 0;

Process::Process(const std::string& name, int id, long long totalLines, const std::string& timeCreated, int coreAssigned)
    : processName(name), processID(id), totalLineOfInstruction(totalLines),
    currLineOfInstruction(0), timeCreated(timeCreated), coreAssigned(coreAssigned) {}

void Process::incrementLine(int core) {
    ConsoleManager::getInstance()->unfinishedProcessList.push_back(this);

    long long busyTime = MainConsole::delaysPerExec;

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
                        for (int i = 0; i < MainConsole::quantumCycles; i++) {
                            // Track the current quantum cycle
                            ScheduleWorker::quantumCycleCounter++; 
                            if (currLineOfInstruction >= totalLineOfInstruction) {
                                break; // Exit loop if done
                            }
                            else {
                                currLineOfInstruction++;
                            }
                        }
                        // TODO: Something for handling ready queue stuff

                        ConsoleManager::getInstance()->waitingProcess(this);
                        break;
                    }
                    this->processCurCycle = MainConsole::curClockCycle;

                    // Memory snapshot file for every quantum cycle
                    if (ScheduleWorker::quantumCycleCounter % MainConsole::quantumCycles == 0) {

                        // ******* Can be commented out later ******* 
                        std::cout << "Quantum cycle " << ScheduleWorker::quantumCycleCounter
                            << " completed. Generating memory snapshot...\n";
                        // ******************************************

                        generateMemorySnapshot(ScheduleWorker::quantumCycleCounter);
                    }
                }
                else {
                    this->setCoreAssigned(core);

                    // Process in CPU but waits for "delay" to finish before executing
                    busyTime--;
                    this->processCurCycle = MainConsole::curClockCycle;
                } 
               
            }
        } // Unlocks mtx when going out of scope

        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // After exiting the loop, handle the finishing process logic
    {
        std::lock_guard<std::mutex> lock(mtx); // Lock for modifying shared resources
        if (currLineOfInstruction >= totalLineOfInstruction) {
            ScheduleWorker::cores[coreAssigned] = -1; // Mark the core as available
            ConsoleManager::getInstance()->addFinishedProcess(this);
            //FileWrite::generateFile(processID, processName, getTimeCreated(), printLogs);
            ScheduleWorker::usedCores--;
        }
    }
}

void Process::processSMI() const {
    std::cout << "Process Name: " << processName << std::endl;
    std::cout << "ID: " << processID << std::endl;
    std::cout << "Current Line of Instruction: " << currLineOfInstruction << "/" << totalLineOfInstruction << std::endl;

    if (currLineOfInstruction >= totalLineOfInstruction) {
        std::cout << "Status: Finished" << std::endl;
    }
    else {
        std::cout << "Status: Running" << std::endl;
    }
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
    if (processesInMemory.empty()) {
        std::cout << "No processes in memory during snapshot at quantum cycle " << quantumCycle << "\n";
    }

    int externalFragmentation = ConsoleManager::getInstance()->calculateExternalFragmentation(ConsoleManager::MAX_MEMORY);
    FileWrite::generateMemorySnapshot(quantumCycle, processesInMemory, externalFragmentation);
}