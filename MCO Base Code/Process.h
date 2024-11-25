#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>

class Process {
public:
    // Constructor to initialize the process with a name, ID, total lines of instruction, and time created
    Process(const std::string& name, int id, long long totalLines, const std::string& timeCreated, int coreAssigned = 5); // Core assignment default 5 for when no core is assigned
    ~Process() = default;

    // Method to check if the process is finished
    bool isFinished() const {
        return currLineOfInstruction >= totalLineOfInstruction;
    }

    // Method to increment the current line of instruction
    void incrementLine(int core);
    
    // Getters
    std::string getName() const { return processName; }
    int getID() const { return processID; }
    long long getCurrentLine() const { return currLineOfInstruction; } 
    long long getTotalLines() const { return totalLineOfInstruction; }
    std::vector<std::string> getPrintLogs();
    std::string getTimeCreated() const { return timeCreated; }
    int getCoreAssigned() const { return coreAssigned; }
    std::string processName;

    // Setter for core
    void setCoreAssigned(int core) { coreAssigned = core; };

    //Process* getProcessByName(const std::string& processName) const;
    static void processSMI();

    // Generate memory snapshot file at quantum cycle
    static void generateMemorySnapshot(int quantumCycle);
    int getEndAddress() const { return endAddress; }
    int getStartAddress() const { return startAddress; }
    // Stter for start and end address
    void setMemoryRange(int start, int end) {
        startAddress = start;
        endAddress = end;
    }

    int getMemoryUsage() const;

private:
    int processCurCycle;
    std::mutex mtx;                     // Mutex for thread safety
    int processID;                      // ID of the process
    long long currLineOfInstruction;    // Current line of instruction
    long long totalLineOfInstruction;   // Total lines of instruction
    std::string timeCreated;            // Time when the process was created
    int coreAssigned;                   // Core assigned to execution of process

    // Temporary (can be deleted later on)
    std::vector<std::string> printLogs;

    // Instance of ScheduleWorker
    //ScheduleWorker* scheduleWorker;

    int startAddress;        
    int endAddress;   
    int memoryUsage;
};