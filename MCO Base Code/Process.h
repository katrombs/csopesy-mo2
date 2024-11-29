#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <unordered_map>


// Paging
struct PageTableEntry {
    int frameNumber; 
    bool valid;       
};

class Process {
public:
    // Constructor to initialize the process with a name, ID, total lines of instruction, and time created
    Process(const std::string& name, int id, long long totalLines, const std::string& timeCreated, int coreAssigned = 5, long long startAddress = 0, long long endAddress = 0); // Core assignment default 5 for when no core is assigned ; startAddress and endAddress default 0
    ~Process() = default;

    void incrementLine(int core);
    
    // Getters
    std::string getName() const { return processName; }
    std::string processName;
    int getID() const { return processID; }
    long long getCurrentLine() const { return currLineOfInstruction; }
    long long getTotalLines() const { return totalLineOfInstruction; }
    std::vector<std::string> getPrintLogs();
    std::string getTimeCreated() const { return timeCreated; }
    int getCoreAssigned() const { return coreAssigned; }
    long long getEndAddress() const { return endAddress; }
    long long getStartAddress() const { return startAddress; }
    long long getMemoryUsage() const;
    long long getMemoryRequired() const;
    long long memoryRequired;
    bool isFinished() const;

    // Setters
    void setCoreAssigned(int core) { coreAssigned = core; };
    void setMemoryRange(long long start, long long memSize);
    void setMemoryRequired(long long memRequired);


    // Paging methods
    void addPage(int pageIndex); 
    void freePages(); 
    std::unordered_map<int, PageTableEntry> pageTable;

    static void processSMI();
    static void generateMemorySnapshot(int quantumCycle);



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

    static long long busyTime;
    //// Memory-related fields
    long long startAddress;
    long long endAddress;

};